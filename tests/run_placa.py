"""tests/run_placa.py - executa os programas da placa no Arduino Uno emulado.

Uso: python3 tests/run_placa.py [dir_testes]   (padrao: tests/placa)

Para cada programa nome.mac do diretorio:
  1. constroi o executavel do Uno a partir do MESMO .ll do PC
     (scripts/construir.sh ... uno, com -DZL_SENTINELA: a biblioteca marca o
     fim do programa com o byte 0x04 na serial);
  2. roda no QEMU (qemu-system-avr -machine uno) com a USART ligada a um
     socket TCP local: envia nome.entrada pela serial e le a serial ate o
     0x04. (TCP funciona em Linux e Windows; no Windows o QEMU nao le a
     serial de um pipe nem de arquivo.)
  3. compara com a saida do PC: nome.esperado sem a linha "[saida N]"
     (ou nome.esperado_placa, quando a saida difere de proposito, como nos
     programas de pino, cujas operacoes o PC so simula). Sem nenhum dos
     dois (programa novo), so mostra a saida da serial.
CONFIG (variavel de ambiente: C0 padrao, C2...) escolhe o pipeline MLIR,
para rodar no Uno tambem os nucleos vetorizados. Tambem confere o tamanho do
executavel (avr-size) e grava um resumo em
build/testes/<dir>-uno-<CONFIG>/tamanhos.txt para docs/placa.md.
"""
import os
import socket
import subprocess
import sys
import time

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DIR = os.path.abspath(sys.argv[1]) if len(sys.argv) > 1 else os.path.join(RAIZ, "tests", "placa")
CONFIG = os.environ.get("CONFIG", "C0")
SAIDA = os.path.join(RAIZ, "build", "testes", f"{os.path.basename(DIR)}-uno-{CONFIG}")
QEMU = os.environ.get("QEMU_AVR", "qemu-system-avr")
AVR_SIZE = os.environ.get("AVR_SIZE", "avr-size")
LIMITE_SEGUNDOS = 30


def porta_livre():
    with socket.socket() as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


def executa_no_qemu(elf, entrada):
    """Roda o .elf no Uno emulado e devolve o texto da serial ate o 0x04."""
    porta = porta_livre()
    processo = subprocess.Popen(
        [QEMU, "-machine", "uno", "-bios", elf, "-nographic", "-monitor", "none",
         "-serial", f"tcp:127.0.0.1:{porta},server=on,wait=on"],
        stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
    )
    recebido = bytearray()
    terminou = False
    conexao = None
    limite = time.time() + LIMITE_SEGUNDOS
    try:
        while conexao is None and time.time() < limite:
            try:
                conexao = socket.create_connection(("127.0.0.1", porta), timeout=1)
            except OSError:
                time.sleep(0.05)
        if conexao is None:
            return "", False
        if entrada:
            conexao.sendall(entrada)
        conexao.settimeout(0.5)
        while time.time() < limite:
            try:
                bloco = conexao.recv(256)
            except socket.timeout:
                continue
            if not bloco:
                break
            if b"\x04" in bloco:
                recebido.extend(bloco[:bloco.index(b"\x04")])
                terminou = True
                break
            recebido.extend(bloco)
    finally:
        if conexao is not None:
            conexao.close()
        processo.kill()
        processo.wait()
    return recebido.decode("utf-8", errors="replace"), terminou


def main():
    os.makedirs(SAIDA, exist_ok=True)
    ambiente = dict(os.environ, CFLAGS_PROG="-DZL_SENTINELA")
    passou = falhou = 0
    tamanhos = []
    for arquivo in sorted(os.listdir(DIR)):
        if not arquivo.endswith(".mac"):
            continue
        nome = arquivo[:-4]
        programa = os.path.join(DIR, arquivo)
        construcao = subprocess.run(
            ["sh", os.path.join(RAIZ, "scripts", "construir.sh"), programa, SAIDA, "uno", CONFIG],
            capture_output=True, text=True, env=ambiente,
        )
        if construcao.returncode != 0:
            falhou += 1
            print(f"FAIL {programa} (nao construiu)\n{construcao.stdout}{construcao.stderr}")
            continue
        esperado_placa = os.path.join(DIR, nome + ".esperado_placa")
        esperado_pc = os.path.join(DIR, nome + ".esperado")
        caminho = esperado_placa if os.path.exists(esperado_placa) else esperado_pc
        esperado = None
        if os.path.exists(caminho):
            with open(caminho, encoding="utf-8") as f:
                linhas = f.read().splitlines(keepends=True)
            esperado = "".join(l for l in linhas if not l.startswith("[saida "))
        entrada = b""
        if os.path.exists(os.path.join(DIR, nome + ".entrada")):
            with open(os.path.join(DIR, nome + ".entrada"), "rb") as f:
                entrada = f.read()
        obtido, terminou = executa_no_qemu(os.path.join(SAIDA, nome + ".elf"), entrada)
        with open(os.path.join(SAIDA, nome + ".serial"), "w", encoding="utf-8", newline="\n") as f:
            f.write(obtido)
        relatorio = subprocess.run(
            [AVR_SIZE, "-C", "--mcu=atmega328p", os.path.join(SAIDA, nome + ".elf")],
            capture_output=True, text=True).stdout
        flash = next((l.split(":", 1)[1].strip() for l in relatorio.splitlines()
                      if l.startswith("Program:")), "?")
        ram = next((l.split(":", 1)[1].strip() for l in relatorio.splitlines()
                    if l.startswith("Data:")), "?")
        tamanhos.append(f"| {nome} | {flash} | {ram} |")
        if esperado is None:
            # programa novo (por exemplo, o inedito da demonstracao): so mostra
            print(f"SAIDA {programa} (Uno/QEMU, sem .esperado; terminou={terminou})")
            print(obtido, end="" if obtido.endswith("\n") else "\n")
        elif terminou and obtido == esperado:
            passou += 1
            print(f"PASS {programa} (Uno/QEMU, saida identica)")
        else:
            falhou += 1
            print(f"FAIL {programa} (terminou={terminou})")
            print("--- esperado\n" + esperado + "--- obtido\n" + obtido)
    with open(os.path.join(SAIDA, "tamanhos.txt"), "w", encoding="utf-8", newline="\n") as f:
        f.write("| Programa | Flash (.text + .data) | RAM estatica (.data + .bss) |\n")
        f.write("|---|---|---|\n")
        f.write("\n".join(tamanhos) + "\n")
    print(f"{os.path.basename(DIR)} (Uno/QEMU, {CONFIG}): {passou} ok, {falhou} falhas")
    return 0 if falhou == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
