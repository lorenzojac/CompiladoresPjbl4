"""tests/run_pico.py - executa os programas da placa na Raspberry Pi Pico 2 W.

Uso: python3 tests/run_pico.py [dir_testes ...]   (padrao: tests/placa)

Como tests/run_placa.py (Arduino Uno no QEMU), mas na placa de verdade,
ligada pela USB. Para cada programa nome.mac do diretorio (so o proprio
diretorio: manual/ e invalidos/ ficam de fora):
  1. constroi o .uf2 da Pico a partir do MESMO .ll do PC
     (scripts/construir.sh ... pico, com -DZL_SENTINELA: a biblioteca marca
     o fim do programa com o byte 0x04 na USB);
  2. grava na placa com scripts/pico.py (toque a 1200 baud -> modo BOOTSEL
     -> copia do .uf2 para a unidade RP2350), abre a porta USB (o programa
     comeca), envia nome.entrada e le ate o 0x04 (limite de 30 s);
  3. compara com a saida do PC: nome.esperado sem a linha "[saida N]"
     (ou nome.esperado_placa, quando a saida difere de proposito, como nos
     programas de pino, cujas operacoes o PC so simula). Sem nenhum dos
     dois (programa novo), so mostra a saida da placa.
CONFIG (variavel de ambiente: C0 padrao, C2...) escolhe o pipeline MLIR.
PICO_PORTA e PICO_UNIDADE escolhem a porta e a unidade BOOTSEL (ver
scripts/pico.py). Sem a placa, sai com codigo 2; com alguma falha, 1.
Saidas em build/testes/<dir>-pico-<CONFIG>/ (nome.uf2, nome.serial).
"""
import os
import subprocess
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, "scripts"))
import pico  # noqa: E402  (scripts/pico.py)

DIRS = [os.path.abspath(d) for d in sys.argv[1:]] or [os.path.join(RAIZ, "tests", "placa")]
CONFIG = os.environ.get("CONFIG", "C0")
LIMITE_SEGUNDOS = 30
ESPERA_BOOTSEL = 10  # no meio dos testes a placa ja esta conectada
SEM_PLACA = "Pico 2 W nao encontrada: conecte a placa (na primeira vez, com BOOTSEL apertado)"


class PlacaSumiu(Exception):
    """A placa desconectou no meio dos testes: nao adianta continuar."""


def esperado_de(diretorio, nome):
    """Saida esperada na placa, sem a linha "[saida N]"; None se nao houver.

    nome.esperado_placa tem preferencia sobre nome.esperado (o do PC)."""
    esperado_placa = os.path.join(diretorio, nome + ".esperado_placa")
    esperado_pc = os.path.join(diretorio, nome + ".esperado")
    caminho = esperado_placa if os.path.exists(esperado_placa) else esperado_pc
    if not os.path.exists(caminho):
        return None
    with open(caminho, encoding="utf-8") as f:
        linhas = f.read().splitlines(keepends=True)
    return "".join(l for l in linhas if not l.startswith("[saida "))


def executa_na_pico(uf2, entrada):
    """Grava o .uf2 e le a USB ate o 0x04: (texto, terminou, erro)."""
    try:
        porta = pico.grava(uf2, espera_bootsel=ESPERA_BOOTSEL)
        return pico.le_ate_eot(porta, entrada, LIMITE_SEGUNDOS), True, ""
    except pico.PlacaNaoEncontrada:
        raise
    except pico.PicoErro as erro:  # tempo esgotado, desconexao, falha na gravacao
        return erro.texto, False, str(erro)


def testa_diretorio(diretorio):
    """Roda os programas de um diretorio; devolve (passou, falhou)."""
    saida = os.path.join(RAIZ, "build", "testes", f"{os.path.basename(diretorio)}-pico-{CONFIG}")
    os.makedirs(saida, exist_ok=True)
    ambiente = dict(os.environ, CFLAGS_PROG="-DZL_SENTINELA")
    passou = falhou = 0
    for arquivo in sorted(os.listdir(diretorio)):
        programa = os.path.join(diretorio, arquivo)
        if not arquivo.endswith(".mac") or not os.path.isfile(programa):
            continue
        nome = arquivo[:-4]
        construcao = subprocess.run(
            ["sh", os.path.join(RAIZ, "scripts", "construir.sh"), programa, saida, "pico", CONFIG],
            capture_output=True, text=True, env=ambiente,
        )
        if construcao.returncode != 0:
            falhou += 1
            print(f"FAIL {programa} (nao construiu)\n{construcao.stdout}{construcao.stderr}")
            continue
        esperado = esperado_de(diretorio, nome)
        entrada = b""
        if os.path.exists(os.path.join(diretorio, nome + ".entrada")):
            with open(os.path.join(diretorio, nome + ".entrada"), "rb") as f:
                entrada = f.read()
        try:
            obtido, terminou, erro = executa_na_pico(os.path.join(saida, nome + ".uf2"), entrada)
        except pico.PlacaNaoEncontrada as erro:
            falhou += 1
            print(f"FAIL {programa} ({erro})")
            print(f"{os.path.basename(diretorio)} (Pico 2 W, {CONFIG}): {passou} ok, "
                  f"{falhou} falhas (interrompido)")
            raise PlacaSumiu()
        with open(os.path.join(saida, nome + ".serial"), "w", encoding="utf-8", newline="\n") as f:
            f.write(obtido)
        detalhe = f"terminou={terminou}" + (f"; {erro}" if erro else "")
        if esperado is None:
            # programa novo (por exemplo, o inedito da demonstracao): so mostra
            print(f"SAIDA {programa} (Pico 2 W, sem .esperado; {detalhe})")
            print(obtido, end="" if obtido.endswith("\n") else "\n")
        elif terminou and obtido == esperado:
            passou += 1
            print(f"PASS {programa} (Pico 2 W, saida identica)")
        else:
            falhou += 1
            print(f"FAIL {programa} ({detalhe})")
            print("--- esperado\n" + esperado + "--- obtido\n" + obtido)
    print(f"{os.path.basename(diretorio)} (Pico 2 W, {CONFIG}): {passou} ok, {falhou} falhas")
    return passou, falhou


def main():
    pico.configura_saida()  # utf-8, como a placa envia
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(line_buffering=True)  # cada programa leva segundos
    try:
        if not pico.espera(lambda: pico.acha_unidade() or pico.acha_porta(), 5):
            print(SEM_PLACA)
            return 2
    except pico.PicoErro as erro:  # pyserial ausente
        print(erro)
        return 1
    falhas = 0
    for diretorio in DIRS:
        try:
            falhas += testa_diretorio(diretorio)[1]
        except PlacaSumiu:
            print(SEM_PLACA)
            return 2
    return 0 if falhas == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
