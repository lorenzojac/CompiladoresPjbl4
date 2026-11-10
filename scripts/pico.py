"""scripts/pico.py - grava e conversa com a Raspberry Pi Pico 2 W pela USB.

Uso: python3 scripts/pico.py portas
     python3 scripts/pico.py gravar programa.uf2 [--porta P] [--espera-bootsel S]
     python3 scripts/pico.py monitor [--porta P] [--ate-eot] [--timeout S] [--entrada ARQ]
     python3 scripts/pico.py executa programa.uf2 [--porta P] [--entrada ARQ] [--timeout S]

A placa (runtime/zl_runtime_pico.c, D49) usa a USB da propria RP2350 como
porta serial (CDC: COMn no Windows, /dev/ttyACMn no Linux; VID 0x2E8A):
  - o programa so comeca quando o PC abre a porta com o sinal DTR (o
    pyserial liga o DTR ao abrir); fechar a porta reinicia a placa, entao
    abrir de novo roda o programa de novo;
  - abrir a porta a 1200 baud reinicia a placa no modo BOOTSEL: ela vira uma
    unidade USB (rotulo RP2350, com INFO_UF2.TXT na raiz); copiar o .uf2
    para ela grava o programa e a placa reinicia sozinha com ele;
  - na primeira gravacao (placa sem um programa nosso), segure o botao
    BOOTSEL enquanto conecta o cabo USB;
  - construido com -DZL_SENTINELA (testes), o fim do programa (normal ou
    erro de execucao) e marcado com o byte 0x04 (EOT).
Variaveis: PICO_PORTA (porta serial) e PICO_UNIDADE (caminho da unidade
BOOTSEL, quando a busca automatica nao a encontra).
Codigos de saida: 0 ok, 1 falha, 2 placa nao encontrada.
tests/run_pico.py importa as funcoes deste arquivo (acha_porta,
acha_unidade, grava, le_ate_eot, ...).
Precisa do pyserial (pip install pyserial; MSYS2:
pacman -S mingw-w64-ucrt-x86_64-python-pyserial).
"""
import argparse
import codecs
import contextlib
import os
import re
import struct
import sys
import threading
import time

try:
    import serial
    from serial.tools import list_ports
except ImportError:  # o aviso so aparece quando a serial for usada
    serial = None
    list_ports = None

VID_RASPBERRY = 0x2E8A
PID_DEBUG_PROBE = 0x000C  # Raspberry Pi Debug Probe: tambem e CDC, mas nao e a placa
BAUD = 115200  # indiferente na USB, mas nunca 1200 (que leva ao BOOTSEL)
BAUD_BOOTSEL = 1200
EOT = b"\x04"

ESPERA_PLACA = 5      # depois de fechar a porta, a placa some por ~1 s
ESPERA_UNIDADE = 15   # do toque a 1200 baud ate a unidade RP2350 aparecer
ESPERA_PORTA = 15     # da copia do .uf2 ate a porta do programa voltar
ESPERA_MONITOR = 20   # monitor/executa: espera a porta aparecer
RETOQUE = 3           # porta ainda la depois do toque: toca de novo

INSTRUCOES = "segure o botao BOOTSEL e conecte o cabo USB"
DICA_UNIDADE = ("se a unidade RP2350 aparece mas nao e encontrada, defina "
                "PICO_UNIDADE com o caminho dela")

# Blocos UF2 (https://github.com/microsoft/uf2) e familias da RP2350.
UF2_BLOCO = 512
UF2_MAGICOS = (0x0A324655, 0x9E5D5157)
UF2_MAGICO_FIM = 0x0AB16F30
UF2_TEM_FAMILIA = 0x00002000
FAMILIA_RP2040 = 0xE48BFF56
FAMILIAS_RP2350 = {
    0xE48BFF57,  # absoluta (bloco extra do SDK para a errata E10)
    0xE48BFF58,  # dados
    0xE48BFF59,  # RP2350 ARM seguro (o nosso)
    0xE48BFF5A,  # RP2350 RISC-V
    0xE48BFF5B,  # RP2350 ARM nao seguro
}


class PicoErro(Exception):
    """Falha ao falar com a placa (saida 1). texto: o que chegou ate entao."""

    def __init__(self, mensagem, texto=""):
        super().__init__(mensagem)
        self.texto = texto


class PlacaNaoEncontrada(PicoErro):
    """Nem porta serial nem unidade BOOTSEL da placa (saida 2)."""


class TempoEsgotado(PicoErro):
    """O programa nao terminou (sem o 0x04) no tempo dado."""


def _calado(_mensagem):
    pass


def espera(condicao, segundos, intervalo=0.2):
    """Chama condicao() ate dar um valor verdadeiro ou o tempo acabar.

    Devolve o ultimo valor (falso se o tempo acabou)."""
    limite = time.monotonic() + segundos
    while True:
        valor = condicao()
        if valor or time.monotonic() >= limite:
            return valor
        time.sleep(intervalo)


# --- porta serial ---------------------------------------------------------

def _exige_pyserial():
    if serial is None:
        raise PicoErro("pyserial nao encontrado: instale com 'pip install pyserial' "
                       "(MSYS2: pacman -S mingw-w64-ucrt-x86_64-python-pyserial)")


def _ordem_natural(texto):
    """Chave de ordenacao com COM3 antes de COM10."""
    return [int(t) if t.isdigit() else t for t in re.split(r"(\d+)", texto)]


def portas_pico():
    """Portas seriais USB da Raspberry Pi (VID 0x2E8A), em ordem natural."""
    _exige_pyserial()
    portas = [p for p in list_ports.comports()
              if p.vid == VID_RASPBERRY and p.pid != PID_DEBUG_PROBE]
    return sorted(portas, key=lambda p: _ordem_natural(p.device))


def _porta_existe(porta):
    if os.name == "nt":
        _exige_pyserial()
        return any(p.device.upper() == porta.upper() for p in list_ports.comports())
    return os.path.exists(porta)  # aceita tambem /dev/serial/by-id/...


def acha_porta(porta=None):
    """Porta serial da Pico, ou None se nao houver.

    Com porta (ou PICO_PORTA), devolve essa porta se ela existir agora; sem,
    a primeira porta com o VID da Raspberry Pi."""
    porta = porta or os.environ.get("PICO_PORTA")
    if porta:
        return porta if _porta_existe(porta) else None
    portas = portas_pico()
    return portas[0].device if portas else None


def espera_porta(porta=None, segundos=ESPERA_MONITOR):
    """Espera a porta da Pico aparecer; PlacaNaoEncontrada se nao aparecer."""
    achada = espera(lambda: acha_porta(porta), segundos)
    if achada:
        return achada
    alvo = porta or os.environ.get("PICO_PORTA")
    mensagem = (f"porta serial {alvo}" if alvo else "porta serial da Pico 2 W") + \
        f" nao encontrada em {segundos:g} s"
    unidade = acha_unidade()
    if unidade:
        mensagem += (f": a placa esta no modo BOOTSEL (unidade {unidade}); grave um "
                     "programa com 'python3 scripts/pico.py gravar programa.uf2'")
    else:
        mensagem += ": conecte a placa pela USB"
    raise PlacaNaoEncontrada(mensagem)


def toque_1200(porta):
    """Abre e fecha a porta a 1200 baud: o SDK reinicia a placa no BOOTSEL.

    Devolve o erro do pyserial (ou None). A placa sumir no meio do toque e
    normal; quem chama confere se a unidade RP2350 apareceu."""
    _exige_pyserial()
    try:
        conexao = serial.Serial(porta, BAUD_BOOTSEL)
    except (serial.SerialException, OSError, ValueError) as erro:
        return erro
    try:
        time.sleep(0.1)
        conexao.close()
    except (serial.SerialException, OSError):
        pass
    return None


def abre_porta(porta, segundos=5):
    """Abre a porta com DTR ligado: o programa da placa comeca a rodar.

    Tenta de novo por alguns segundos: logo depois de aparecer na lista, a
    porta pode ainda nao abrir (o Windows ainda instalando o dispositivo)."""
    _exige_pyserial()
    limite = time.monotonic() + segundos
    while True:
        conexao = serial.Serial()
        conexao.port = porta
        conexao.baudrate = BAUD
        conexao.timeout = 0.1
        conexao.dtr = True
        conexao.rts = True
        try:
            conexao.open()
            return conexao
        except (serial.SerialException, OSError) as erro:
            if time.monotonic() >= limite:
                dica = "outro programa (monitor serial?) esta usando a porta?"
                if os.name != "nt":
                    dica += " no Linux, o usuario precisa estar no grupo dialout"
                raise PicoErro(f"nao consegui abrir {porta}: {erro} ({dica})")
        time.sleep(0.25)


def _fecha(conexao):
    """Fecha a porta (a placa reinicia e volta a esperar um terminal)."""
    try:
        conexao.close()
    except (serial.SerialException, OSError):
        pass


def _envia_em_fundo(conexao, dados):
    """Envia a entrada numa thread, lendo a saida ao mesmo tempo.

    Se o programa escreve muito antes de ler, escrever e so depois ler
    travaria os dois lados."""
    def envia():
        try:
            conexao.write(dados)
            conexao.flush()
        except Exception:  # porta fechada no fim do programa: sobrou entrada
            pass

    linha = threading.Thread(target=envia, daemon=True)
    linha.start()
    return linha


def _repassa_stdin(conexao):
    """Monitor interativo: cada linha digitada vai para o programa."""
    try:
        for linha in sys.stdin:
            conexao.write(linha.encode("utf-8"))
    except Exception:  # porta fechada ou stdin indisponivel: para de repassar
        pass


def _le_serial(conexao, ate_eot=False, timeout=None, ao_receber=None):
    """Le a serial como texto: utf-8 (bytes invalidos trocados), sem os CR.

    Com ate_eot, para no byte 0x04 (que nao entra no texto). Devolve
    (texto, achou_eot); sem ate_eot so sai por timeout (TempoEsgotado),
    desconexao (PicoErro) ou Ctrl+C. ao_receber(pedaco) recebe o texto
    conforme chega."""
    decodificador = codecs.getincrementaldecoder("utf-8")(errors="replace")
    partes = []
    limite = None if timeout is None else time.monotonic() + timeout

    def entrega(pedaco):
        pedaco = pedaco.replace("\r", "")
        if pedaco:
            partes.append(pedaco)
            if ao_receber is not None:
                ao_receber(pedaco)

    while True:
        if limite is not None and time.monotonic() >= limite:
            entrega(decodificador.decode(b"", True))
            raise TempoEsgotado(f"tempo esgotado: {timeout:g} s sem o fim do programa"
                                + (" (byte 0x04; o .uf2 foi construido com "
                                   "CFLAGS_PROG=-DZL_SENTINELA?)" if ate_eot else ""),
                                "".join(partes))
        try:
            bloco = conexao.read(conexao.in_waiting or 1)
        except (serial.SerialException, OSError) as erro:
            entrega(decodificador.decode(b"", True))
            raise PicoErro(f"a placa desconectou ({erro})", "".join(partes))
        if not bloco:
            continue
        fim = ate_eot and EOT in bloco
        if fim:
            bloco = bloco[:bloco.index(EOT)]
        entrega(decodificador.decode(bloco, fim))
        if fim:
            return "".join(partes), True


def le_ate_eot(porta=None, entrada=None, timeout=30, ao_receber=None):
    """Roda o programa ja gravado e devolve a saida dele.

    Espera a porta (ate 20 s; porta None = a primeira da Pico), abre (o
    programa comeca), envia entrada (bytes ou str) e le ate o byte 0x04 do
    -DZL_SENTINELA. O texto volta sem o 0x04 e sem os CR. Fechar a porta no
    fim reinicia a placa. Mais de timeout segundos lendo (None = sem
    limite): TempoEsgotado, com o texto parcial em .texto."""
    porta = espera_porta(porta, ESPERA_MONITOR)
    conexao = abre_porta(porta)
    try:
        if entrada:
            _envia_em_fundo(conexao, entrada.encode("utf-8") if isinstance(entrada, str)
                            else bytes(entrada))
        texto, _ = _le_serial(conexao, True, timeout, ao_receber)
        return texto
    finally:
        _fecha(conexao)


# --- unidade BOOTSEL --------------------------------------------------------

def le_board_id(texto):
    """Valor da linha "Board-ID:" de um INFO_UF2.TXT (None se nao houver)."""
    for linha in texto.splitlines():
        chave, separador, valor = linha.partition(":")
        if separador and chave.strip().lower() == "board-id":
            return valor.strip()
    return None


def eh_info_rp2350(texto):
    """O INFO_UF2.TXT e de uma RP2350 (Pico 2) no BOOTSEL? RP2040 nao serve."""
    board_id = le_board_id(texto)
    if board_id is not None:
        return board_id.upper().startswith("RP2350")
    return "RP2350" in texto.upper()


def _arquivo_info(pasta):
    """Caminho do INFO_UF2.TXT na raiz da pasta (qualquer caixa), ou None."""
    try:
        nomes = os.listdir(pasta)
    except OSError:
        return None
    for nome in nomes:
        if nome.upper() == "INFO_UF2.TXT":
            return os.path.join(pasta, nome)
    return None


def eh_unidade_rp2350(pasta):
    """A pasta e a raiz de uma unidade BOOTSEL da RP2350?"""
    info = _arquivo_info(pasta)
    if info is None:
        return False
    try:
        with open(info, encoding="utf-8", errors="replace") as arquivo:
            return eh_info_rp2350(arquivo.read(4096))
    except OSError:
        return False


@contextlib.contextmanager
def _sem_janela_de_disco():
    """Windows: leitor de cartao vazio nao abre a janela "insira um disco"."""
    if os.name != "nt":
        yield
        return
    import ctypes
    kernel32 = ctypes.windll.kernel32
    antigo = ctypes.c_uint()
    kernel32.SetThreadErrorMode(0x0001, ctypes.byref(antigo))  # SEM_FAILCRITICALERRORS
    try:
        yield
    finally:
        kernel32.SetThreadErrorMode(antigo.value, None)


def _raizes_windows():
    import ctypes
    kernel32 = ctypes.windll.kernel32
    mascara = kernel32.GetLogicalDrives()
    raizes = []
    for i in range(26):
        if mascara & (1 << i):
            raiz = chr(ord("A") + i) + ":\\"
            # 2 removivel, 3 fixa, 6 RAM; rede e CD (lentos) ficam de fora
            if kernel32.GetDriveTypeW(raiz) in (2, 3, 6):
                raizes.append(raiz)
    return raizes


def _pastas_de_montagem(bases=None):
    """Linux: /media/$USER/*, /run/media/$USER/*, /media/* (e /Volumes/*)."""
    if bases is None:
        usuario = os.environ.get("USER") or os.environ.get("LOGNAME")
        bases = []
        if usuario:
            bases += [os.path.join("/media", usuario), os.path.join("/run/media", usuario)]
        bases += ["/media", "/Volumes"]
    pastas = []
    for base in bases:
        try:
            nomes = sorted(os.listdir(base))
        except OSError:
            continue
        pastas += [os.path.join(base, nome) for nome in nomes]
    return pastas


def acha_unidade():
    """Raiz da unidade BOOTSEL da RP2350 (ex.: E:\\ ou /media/eu/RP2350), ou None.

    Com PICO_UNIDADE, so ela e considerada (vale quando tem INFO_UF2.TXT)."""
    fixa = os.environ.get("PICO_UNIDADE")
    if fixa:
        return fixa if _arquivo_info(fixa) else None
    with _sem_janela_de_disco():
        candidatas = _raizes_windows() if os.name == "nt" else _pastas_de_montagem()
        for pasta in candidatas:
            if eh_unidade_rp2350(pasta):
                return pasta
    return None


def entra_bootsel(porta=None, espera_bootsel=60, avisa=None):
    """Leva a placa ao modo BOOTSEL e devolve a raiz da unidade RP2350.

    Unidade ja montada: usa. Porta serial do programa: toque a 1200 baud
    (de novo a cada 3 s se a porta continuar la). Nem porta nem unidade em
    5 s (a placa pode estar reiniciando): pede para segurar o BOOTSEL e
    espera mais espera_bootsel segundos (PlacaNaoEncontrada)."""
    avisa = avisa or _calado
    inicio = time.monotonic()
    primeiro_toque = ultimo_toque = None
    porta_tocada = erro_toque = None
    avisou = False
    while True:
        unidade = acha_unidade()
        if unidade:
            return unidade
        agora = time.monotonic()
        achada = acha_porta(porta)
        if achada and (ultimo_toque is None or agora - ultimo_toque >= RETOQUE):
            if primeiro_toque is None:
                avisa(f"reiniciando a placa ({achada}) no modo BOOTSEL")
                primeiro_toque = agora
            porta_tocada = achada
            erro_toque = toque_1200(achada)
            ultimo_toque = agora
        elif not achada and primeiro_toque is None and not avisou and \
                agora - inicio >= ESPERA_PLACA:
            avisa(f"Pico 2 W nao encontrada: {INSTRUCOES} "
                  f"(esperando ate {espera_bootsel:g} s)")
            avisou = True
        if primeiro_toque is not None and agora - primeiro_toque >= ESPERA_UNIDADE:
            mensagem = (f"a placa nao entrou no modo BOOTSEL (a unidade RP2350 nao "
                        f"apareceu em {ESPERA_UNIDADE} s)")
            if erro_toque is not None:
                mensagem += (f"; ao abrir {porta_tocada} a 1200 baud: {erro_toque} "
                             "(outro programa esta usando a porta?)")
            else:
                mensagem += f"; se o programa gravado nao aceita o toque a 1200 baud, {INSTRUCOES}"
            raise PicoErro(mensagem + "; " + DICA_UNIDADE)
        if primeiro_toque is None and agora - inicio >= ESPERA_PLACA + espera_bootsel:
            raise PlacaNaoEncontrada("Pico 2 W nao encontrada (nem porta serial nem "
                                     f"unidade RP2350): {INSTRUCOES}; {DICA_UNIDADE}")
        time.sleep(0.2)


# --- gravacao ---------------------------------------------------------------

def confere_uf2(dados, nome="arquivo"):
    """Levanta PicoErro se os bytes nao forem um .uf2 para a RP2350."""
    if not dados or len(dados) % UF2_BLOCO:
        raise PicoErro(f"{nome} nao e um .uf2 (tamanho {len(dados)} nao e multiplo de "
                       f"{UF2_BLOCO}; use o .uf2, nao o .elf)")
    familias = set()
    for inicio in range(0, len(dados), UF2_BLOCO):
        magico1, magico2, flags = struct.unpack_from("<3I", dados, inicio)
        familia, = struct.unpack_from("<I", dados, inicio + 28)
        fim, = struct.unpack_from("<I", dados, inicio + UF2_BLOCO - 4)
        if (magico1, magico2) != UF2_MAGICOS or fim != UF2_MAGICO_FIM:
            raise PicoErro(f"{nome} nao e um .uf2 (bloco {inicio // UF2_BLOCO} invalido; "
                           "use o .uf2, nao o .elf)")
        if flags & UF2_TEM_FAMILIA:
            familias.add(familia)
    if familias and not familias & FAMILIAS_RP2350:
        if FAMILIA_RP2040 in familias:
            raise PicoErro(f"{nome} e para a RP2040 (Pico 1), nao para a RP2350 (Pico 2 W)")
        raise PicoErro(f"{nome} nao e para a RP2350 (familias UF2: "
                       + ", ".join(f"0x{f:08x}" for f in sorted(familias)) + ")")


def le_uf2(uf2):
    """Le e confere o .uf2 (antes de mexer na placa)."""
    try:
        with open(uf2, "rb") as arquivo:
            dados = arquivo.read()
    except OSError as erro:
        raise PicoErro(f"nao consegui ler {uf2}: {erro}")
    confere_uf2(dados, uf2)
    return dados


def copia_uf2(uf2, unidade, dados=None):
    """Copia o .uf2 para a unidade BOOTSEL; devolve o caminho de destino.

    A placa reinicia ao receber o ultimo bloco, entao um erro no fim da
    copia (a unidade sumiu) e normal e e ignorado."""
    if dados is None:
        dados = le_uf2(uf2)
    destino = os.path.join(unidade, os.path.basename(uf2))
    try:
        arquivo = open(destino, "wb")
    except OSError as erro:
        raise PicoErro(f"nao consegui criar {destino}: {erro}")
    entregue = False
    try:
        with arquivo:
            arquivo.write(dados)
            entregue = True
            arquivo.flush()
            os.fsync(arquivo.fileno())
    except OSError as erro:
        sumiu = espera(lambda: not os.path.isdir(unidade), 2)
        if not (entregue or sumiu):
            raise PicoErro(f"falha ao copiar para {unidade}: {erro}")
    return destino


def _porta_do_programa(porta, antes):
    """Depois da gravacao: a porta pedida, ou uma porta da Pico que nao
    existia antes (outra Pico ja conectada nao conta)."""
    pedida = porta or os.environ.get("PICO_PORTA")
    if pedida and _porta_existe(pedida):
        return pedida
    novas = [p.device for p in portas_pico() if p.device not in antes]
    return novas[0] if novas else None


def grava(uf2, porta=None, espera_bootsel=60, espera_porta=ESPERA_PORTA, avisa=None):
    """Grava o .uf2 na placa e devolve a porta serial do programa novo.

    Unidade BOOTSEL presente: copia. Senao, toque a 1200 baud na porta da
    Pico; sem porta, pede o BOOTSEL e espera ate espera_bootsel segundos
    (PlacaNaoEncontrada). Depois da copia espera a porta voltar (ate
    espera_porta segundos; 0 = nao espera e devolve None)."""
    avisa = avisa or _calado
    dados = le_uf2(uf2)
    unidade = entra_bootsel(porta, espera_bootsel, avisa)
    antes = {p.device for p in portas_pico()}
    avisa(f"gravando {os.path.basename(uf2)} ({len(dados)} bytes) em {unidade}")
    copia_uf2(uf2, unidade, dados)
    if not espera_porta:
        return None
    # A placa so reinicia quando o Windows termina de gravar a unidade; ate a
    # unidade sumir, a lista de portas pode ainda mostrar a porta antiga
    # (testado na placa: abrir cedo demais leu uma saida vazia).
    espera(lambda: not eh_unidade_rp2350(unidade), espera_porta)
    nova = espera(lambda: _porta_do_programa(porta, antes), espera_porta)
    if not nova:
        raise PicoErro(f"o .uf2 foi copiado para {unidade}, mas a porta serial da placa "
                       f"nao voltou em {espera_porta:g} s (desconecte e conecte o cabo)")
    avisa(f"gravado; o programa espera um terminal na porta {nova}")
    return nova


# --- linha de comando ---------------------------------------------------------

_ultimo_caractere = "\n"


def _mostra(texto):
    global _ultimo_caractere
    sys.stdout.write(texto)
    sys.stdout.flush()
    if texto:
        _ultimo_caractere = texto[-1]


def _avisa(mensagem):
    """Mensagens da ferramenta vao para stderr; stdout fica so com o programa."""
    global _ultimo_caractere
    if _ultimo_caractere != "\n":
        sys.stdout.flush()
        sys.stderr.write("\n")
        _ultimo_caractere = "\n"
    print(f"pico.py: {mensagem}", file=sys.stderr, flush=True)


def _le_entrada(caminho):
    if not caminho:
        return None
    try:
        with open(caminho, "rb") as arquivo:
            return arquivo.read()
    except OSError as erro:
        raise PicoErro(f"nao consegui ler {caminho}: {erro}")


def _cmd_portas(_args):
    portas = portas_pico()
    unidade = acha_unidade()
    if portas:
        print(f"Portas seriais da Pico (USB {VID_RASPBERRY:04X}):")
        for p in portas:
            serie = f"  serie {p.serial_number}" if p.serial_number else ""
            print(f"  {p.device}  {p.vid:04X}:{p.pid or 0:04X}  {p.description}{serie}")
    else:
        print("Nenhuma porta serial da Pico.")
    print(f"Unidade BOOTSEL (RP2350): {unidade}" if unidade
          else "Nenhuma unidade BOOTSEL (RP2350).")
    if not portas and not unidade:
        print("Conecte a placa (na primeira vez, com BOOTSEL apertado).")
        return 2
    return 0


def _cmd_gravar(args):
    grava(args.uf2, args.porta, args.espera_bootsel, avisa=_avisa)
    return 0


def _cmd_monitor(args):
    entrada = _le_entrada(args.entrada)
    porta = espera_porta(args.porta, ESPERA_MONITOR)
    conexao = abre_porta(porta)
    _avisa(f"conectado a {porta}" + (" (ate o 0x04)" if args.ate_eot else
                                     " (o que voce digitar vai para o programa; Ctrl+C sai)"))
    try:
        if entrada:
            _envia_em_fundo(conexao, entrada)
        if not args.ate_eot:
            threading.Thread(target=_repassa_stdin, args=(conexao,), daemon=True).start()
        _le_serial(conexao, args.ate_eot, args.timeout or None, _mostra)
        return 0
    except KeyboardInterrupt:
        _avisa("monitor encerrado (fechar a porta reinicia a placa; abra de novo "
               "para rodar o programa outra vez)")
        return 1 if args.ate_eot else 0
    finally:
        _fecha(conexao)


def _cmd_executa(args):
    entrada = _le_entrada(args.entrada)
    porta = grava(args.uf2, args.porta, args.espera_bootsel, avisa=_avisa)
    le_ate_eot(porta, entrada, args.timeout or None, _mostra)
    return 0


class _Parser(argparse.ArgumentParser):
    """Erro de uso sai com 1: o 2 fica reservado para "placa nao encontrada"."""

    def error(self, message):
        self.print_usage(sys.stderr)
        self.exit(1, f"{self.prog}: erro: {message}\n")


def _novo(construtor, nome=None, **opcoes):
    """Parser com a ajuda (-h) em portugues."""
    if nome is None:
        parser = construtor(add_help=False, **opcoes)
    else:
        parser = construtor(nome, add_help=False, **opcoes)
    parser.add_argument("-h", "--help", action="help", help="mostra esta ajuda e sai")
    return parser


def _tempo(texto):
    valor = float(texto)
    if valor < 0:
        raise argparse.ArgumentTypeError("o tempo nao pode ser negativo")
    return valor


def _parser():
    parser = _novo(
        _Parser, prog="pico.py",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description="Grava programas na Raspberry Pi Pico 2 W e conversa com eles pela USB.",
        epilog="Variaveis: PICO_PORTA (porta serial), PICO_UNIDADE (unidade BOOTSEL).\n"
               "Saida: 0 ok, 1 falha, 2 placa nao encontrada.\n"
               "Primeira gravacao: segure o botao BOOTSEL enquanto conecta o cabo USB.")
    comandos = parser.add_subparsers(dest="comando", metavar="comando",
                                     title="comandos")
    comandos.required = True

    def porta(sub):
        sub.add_argument("--porta", metavar="P",
                         help="porta serial da placa (padrao: PICO_PORTA ou a primeira "
                              "com o VID 2E8A)")

    def espera_bootsel(sub):
        sub.add_argument("--espera-bootsel", type=_tempo, default=60, metavar="S",
                         help="sem porta nem unidade, quantos segundos esperar o BOOTSEL "
                              "(padrao: 60)")

    sub = _novo(comandos.add_parser, "portas",
                help="lista as portas seriais da Pico e a unidade BOOTSEL",
                description="Lista as portas seriais da Pico (VID 2E8A) e a unidade "
                            "BOOTSEL (RP2350), se houver.")
    sub.set_defaults(funcao=_cmd_portas)

    sub = _novo(comandos.add_parser, "gravar", help="grava um .uf2 na placa",
                description="Grava o .uf2: toque a 1200 baud na porta (modo BOOTSEL), "
                            "copia para a unidade RP2350 e espera a porta voltar.")
    sub.add_argument("uf2", help="programa (.uf2)")
    porta(sub)
    espera_bootsel(sub)
    sub.set_defaults(funcao=_cmd_gravar)

    sub = _novo(comandos.add_parser, "monitor",
                help="terminal serial (abrir a porta roda o programa)",
                description="Abre a porta (o programa comeca) e mostra o que chega. Sem "
                            "--ate-eot, o que for digitado vai para o programa; Ctrl+C sai "
                            "(fechar a porta reinicia a placa).")
    porta(sub)
    sub.add_argument("--ate-eot", action="store_true",
                     help="para no byte 0x04 (programas construidos com -DZL_SENTINELA)")
    sub.add_argument("--timeout", type=_tempo, default=None, metavar="S",
                     help="desiste depois de S segundos lendo (erro; padrao: sem limite)")
    sub.add_argument("--entrada", metavar="ARQ",
                     help="envia este arquivo logo depois de abrir (para programas que "
                          "chamam le)")
    sub.set_defaults(funcao=_cmd_monitor)

    sub = _novo(comandos.add_parser, "executa",
                help="grava o .uf2 e mostra a saida ate o 0x04",
                description="gravar + monitor --ate-eot: saida 0 se o programa terminou "
                            "(0x04), 1 se o tempo acabou.")
    sub.add_argument("uf2", help="programa (.uf2) construido com -DZL_SENTINELA")
    porta(sub)
    sub.add_argument("--entrada", metavar="ARQ", help="entrada do programa")
    sub.add_argument("--timeout", type=_tempo, default=30, metavar="S",
                     help="limite para o programa terminar (padrao: 30; 0 = sem limite)")
    espera_bootsel(sub)
    sub.set_defaults(funcao=_cmd_executa)
    return parser


def configura_saida():
    """stdout em utf-8, como a placa envia (no Windows, um pipe, como o do Git
    Bash, usaria cp1252), sem quebrar com caracteres que nao cabem."""
    for fluxo in (sys.stdout, sys.stderr):
        if hasattr(fluxo, "reconfigure"):
            if os.environ.get("PYTHONIOENCODING"):
                fluxo.reconfigure(errors="replace")
            else:
                fluxo.reconfigure(encoding="utf-8", errors="replace")


def main(argv=None):
    configura_saida()
    args = _parser().parse_args(argv)
    try:
        return args.funcao(args)
    except PlacaNaoEncontrada as erro:
        _avisa(erro)
        return 2
    except PicoErro as erro:
        _avisa(erro)
        return 1
    except KeyboardInterrupt:
        _avisa("interrompido")
        return 1


if __name__ == "__main__":
    sys.exit(main())
