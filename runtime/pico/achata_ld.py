"""Junta um script de linker e todos os seus INCLUDE em um unico arquivo.

O GNU ld 2.47 do MSYS2 (Windows) recusa ligar com mais de um arquivo de
script ("linker script file ... appears multiple times"), contando tambem os
INCLUDE: no Windows todos os arquivos parecem o mesmo para a verificacao de
duplicados. Os scripts do Pico SDK sao divididos em ~20 arquivos .incl, entao
o CMake da placa (runtime/pico/CMakeLists.txt) usa este programa para gerar
um script unico com o mesmo texto que o ld leria (D48).

Uso:
  python achata_ld.py saida.ld principal.ld [--antes arq.ld] -L dir [-L dir...]

--antes: script copiado antes do principal (o default_locations.ld, que o SDK
passa com --script); -L: pastas onde procurar os INCLUDE, na ordem do ld.
"""
import argparse
import os
import re
import sys

INCLUDE = re.compile(r'^\s*INCLUDE\s+"?([^"\s;]+)"?\s*;?\s*$')
PROFUNDIDADE_MAXIMA = 32


def acha(nome, pastas):
    if os.path.isabs(nome) and os.path.isfile(nome):
        return nome
    for pasta in pastas:
        caminho = os.path.join(pasta, nome)
        if os.path.isfile(caminho):
            return caminho
    sys.exit(f"achata_ld: INCLUDE \"{nome}\" nao encontrado em {pastas}")


def fim_em_comentario(linha, dentro):
    """Diz se, ao fim da linha, ainda estamos dentro de um /* ... */."""
    posicao = 0
    while True:
        marca = "*/" if dentro else "/*"
        achado = linha.find(marca, posicao)
        if achado < 0:
            return dentro
        dentro = not dentro
        posicao = achado + 2


def expande(caminho, pastas, pilha, saida):
    if len(pilha) > PROFUNDIDADE_MAXIMA or caminho in pilha:
        sys.exit(f"achata_ld: INCLUDE recursivo em {caminho}")
    pilha.append(caminho)
    saida.append(f"/* ---- inicio de {os.path.basename(caminho)} ---- */\n")
    em_comentario = False
    with open(caminho, encoding="utf-8") as arquivo:
        for linha in arquivo:
            # INCLUDE dentro de comentario (exemplos do SDK) fica como texto.
            achado = None if em_comentario else INCLUDE.match(linha)
            if achado:
                expande(acha(achado.group(1), pastas), pastas, pilha, saida)
            else:
                saida.append(linha if linha.endswith("\n") else linha + "\n")
                em_comentario = fim_em_comentario(linha, em_comentario)
    saida.append(f"/* ---- fim de {os.path.basename(caminho)} ---- */\n")
    pilha.pop()


def main():
    args = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    args.add_argument("saida")
    args.add_argument("principal")
    args.add_argument("--antes", action="append", default=[])
    args.add_argument("-L", dest="pastas", action="append", default=[])
    opcoes = args.parse_args()

    pastas = [os.path.dirname(os.path.abspath(opcoes.principal))]
    pastas += opcoes.pastas
    texto = [
        "/* Gerado por runtime/pico/achata_ld.py a partir dos scripts do\n",
        " * Pico SDK: o mesmo texto, em um arquivo so (D48). */\n",
    ]
    for arquivo in opcoes.antes + [opcoes.principal]:
        expande(acha(arquivo, pastas), pastas, [], texto)

    novo = "".join(texto)
    if os.path.isfile(opcoes.saida):
        with open(opcoes.saida, encoding="utf-8") as atual:
            if atual.read() == novo:
                return
    with open(opcoes.saida, "w", encoding="utf-8", newline="\n") as destino:
        destino.write(novo)


if __name__ == "__main__":
    main()
