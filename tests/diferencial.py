"""tests/diferencial.py - teste diferencial do gerador de codigo.

Uso: python3 tests/diferencial.py [quantidade] [semente]     (ou: make diferencial)
     make test roda uma rodada curta (60 programas, semente fixa).

Gera programas Macaronica VALIDOS aleatorios e, ao mesmo tempo, os executa
num interpretador de referencia escrito aqui, que implementa a semantica
decidida pelo grupo de forma independente do compilador:
  - inteira: complemento de dois de 32 bits (estouro da a volta, D43);
    // trunca para zero; ^ por multiplicacoes com estouro;
  - flut: cada operacao arredondada para float de 32 bits; duplocarpado:
    double; promocao inteira < flut < duplocarpado (D16/D17);
  - '/' sempre real, flut ou duplocarpado (D18/D38); literal decimal
    promovido a duplocarpado convertido a partir do texto (D38);
  - impressao com 4 casas (flut) e 8 (duplocarpado), metade para longe do
    zero, como runtime/zl_runtime_comum.c (D42).
Cada programa e compilado (macaronica -o, llvm-as, opt -passes=verify) e
executado com lli, clang -O0 e clang -O2; as tres saidas devem ser iguais a
do interpretador. Divisores sao sempre nao nulos (x*x+1) e indices sempre
validos, entao nenhum programa deve terminar com erro de execucao.
"""
import math
import os
import random
import struct
import subprocess
import sys
import tempfile

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = ".exe" if os.name == "nt" else ""
MACARONICA = os.path.join(RAIZ, "macaronica" + EXE)
CLANG = os.environ.get("CLANG", "clang")
LLI = os.environ.get("LLI", "lli")
OPT = os.environ.get("OPT", "opt")
LLVM_AS = os.environ.get("LLVM_AS", "llvm-as")
RUNTIME = os.path.join(RAIZ, "runtime", "zl_runtime_posix.c")

I, F, D, B = "inteira", "flut", "duplocarpado", "bool"
RANK = {I: 0, F: 1, D: 2}


class Invalido(Exception):
    """O programa sorteado saiu da faixa segura (ex.: estouro para inf)."""


def wrap(v):
    v &= 0xFFFFFFFF
    return v - (1 << 32) if v & 0x80000000 else v


def f32(x):
    if math.isnan(x) or math.isinf(x):
        raise Invalido()
    if abs(x) >= 3.4e38:
        raise Invalido()
    return struct.unpack("f", struct.pack("f", x))[0]


def wider(a, b):
    return a if RANK[a] >= RANK[b] else b


class Valor:
    """Valor tipado do interpretador; literal guarda o texto decimal."""

    def __init__(self, tipo, v, literal=None):
        self.tipo, self.v, self.literal = tipo, v, literal


def converte(valor, para):
    if valor.tipo == para:
        return valor
    if valor.tipo == I and para == F:
        return Valor(F, f32(float(valor.v)))
    if valor.tipo == I and para == D:
        return Valor(D, float(valor.v))
    if valor.tipo == F and para == D:
        if valor.literal is not None:
            return Valor(D, float(valor.literal))
        return Valor(D, valor.v)
    raise AssertionError((valor.tipo, para))


def arred(tipo, x):
    if math.isnan(x) or math.isinf(x) or abs(x) > 1e300:
        raise Invalido()
    return f32(x) if tipo == F else x


def formata(tipo, v):
    if tipo == I:
        return str(v)
    if tipo == B:
        return "VER" if v else "FAL"
    casas = 4 if tipo == F else 8
    return formata_real(v, casas)


def formata_real(valor, casas):
    if valor != valor:
        return "nan"
    if valor - valor != 0.0:
        return "inf" if valor > 0 else "-inf"
    potencia = 10 ** casas
    escala = 1.0
    for _ in range(casas):
        escala *= 10.0
    absoluto = -valor if valor < 0 else valor
    if absoluto * escala < 9.0e15:
        escalado = int(absoluto * escala + 0.5)
        sinal = "-" if valor < 0 and escalado != 0 else ""
        return f"{sinal}{escalado // potencia}.{escalado % potencia:0{casas}d}"
    # Notacao cientifica, como escreve_real em zl_runtime_comum.c.
    expoente = 0
    while absoluto >= 10.0:
        absoluto /= 10.0
        expoente += 1
    if int(absoluto * escala + 0.5) >= 10 * potencia:
        absoluto /= 10.0
        expoente += 1
    return formata_real(-absoluto if valor < 0 else absoluto, casas) + f"e+{expoente}"


# ---------------------------------------------------------------------------
# Expressoes: cada no e (texto, funcao que avalia no ambiente)


class Gerador:
    def __init__(self, rng):
        self.rng = rng
        self.globais = {I: ["g0", "g1"], F: ["gf"], D: ["gd"], B: ["gb"]}
        self.ambiente = {"g0": Valor(I, 0), "g1": Valor(I, 0), "gf": Valor(F, 0.0),
                         "gd": Valor(D, 0.0), "gb": Valor(B, False)}
        self.vetor = [0] * 5
        self.laco = None
        self.saida = []

    def literal(self, tipo):
        r = self.rng
        if tipo == I:
            n = r.choice([0, 1, 2, 3, 7, 10, 100, r.randint(0, 50000)])
            return str(n), (lambda env, n=n: Valor(I, n))
        if tipo in (F, D):
            texto = r.choice(["0.5", "1.25", "0.1", "2.0", "3.75", "0.3", "10.0"])
            return texto, (lambda env, t=texto: Valor(F, f32(float(t)), literal=t))
        lit = r.choice(["VER", "FAL"])
        return lit, (lambda env, b=(lit == "VER"): Valor(B, b))

    def variavel(self, tipo):
        nomes = list(self.globais.get(tipo, []))
        if tipo == I and self.laco:
            nomes.append(self.laco)
        nome = self.rng.choice(nomes)
        return nome, (lambda env, n=nome: env[n])

    def num(self, tipo, prof):
        """Expressao numerica cujo tipo e exatamente 'tipo'."""
        r = self.rng
        if prof <= 0 or r.random() < 0.25:
            # Folha com o tipo estatico exato: nao existe literal duplocarpado
            # (literal decimal e flut, D15), so a variavel gd.
            if tipo == D:
                return self.variavel(D)
            return r.choice([self.literal, self.variavel])(tipo)
        escolha = r.random()
        if tipo == I:
            if escolha < 0.45:
                op = r.choice(["+", "-", "*"])
                a, ea = self.num(I, prof - 1)
                b, eb = self.num(I, prof - 1)
                return f"({a} {op} {b})", self.bin_int(op, ea, eb)
            if escolha < 0.6:
                a, ea = self.num(I, prof - 1)
                b, eb = self.num(I, prof - 1)
                texto = f"({a} // ({b} * {b} + 1))"
                return texto, self.div_int(ea, eb)
            if escolha < 0.7:
                a, ea = self.num(I, prof - 1)
                k = r.randint(0, 4)
                return f"({a} ^ {k})", self.pot_int(ea, k)
            if escolha < 0.8:
                a, ea = self.num(I, prof - 1)
                return f"(-{a})", (lambda env: Valor(I, wrap(-ea(env).v)))
            if escolha < 0.9:
                idx = self.indice()
                return f"gv{{{idx[0]}}}", (lambda env: Valor(I, self.vetor[idx[1](env)]))
            a, ea = self.num(I, prof - 1)
            b, eb = self.num(I, prof - 1)
            return f"fi({a}, {b})", (lambda env: self.chama_fi(ea(env), eb(env)))
        # tipos reais
        if escolha < 0.4:
            op = r.choice(["+", "-", "*"])
            ta, tb = self.par_de_tipos(tipo)
            a, ea = self.num(ta, prof - 1)
            b, eb = self.num(tb, prof - 1)
            return f"({a} {op} {b})", self.bin_real(op, tipo, ea, eb)
        if escolha < 0.6:
            # '/' tem tipo max(flut, tipos dos operandos) (D18/D38).
            ta, tb = self.par_de_tipos(tipo, permite_int=True)
            a, ea = self.num(ta, prof - 1)
            b, eb = self.num(tb, prof - 1)
            return f"({a} / ({b} * {b} + 1))", self.div_real(tipo, tb, ea, eb)
        if escolha < 0.75:
            # raiz(x) tem tipo max(flut, tipo de x): para duplocarpado, x e D.
            ta = D if tipo == D else r.choice([I, F])
            a, ea = self.num(ta, prof - 1)
            return f"raiz({a} * {a})", self.raiz(tipo, ta, ea)
        if escolha < 0.85 and tipo == F:
            a, ea = self.num(F, prof - 1)
            return f"ff({a})", (lambda env: self.chama_ff(ea(env)))
        a, ea = self.num(tipo, prof - 1)
        return f"({a} ^ 2)", self.quadrado(tipo, ea)

    def par_de_tipos(self, tipo, permite_int=False):
        opcoes = [I, F] if tipo == F else [I, F, D]
        a = self.rng.choice(opcoes)
        b = self.rng.choice(opcoes)
        if tipo == D and D not in (a, b):
            a = D
        if tipo == F and F not in (a, b) and not permite_int:
            a = F
        return a, b

    def indice(self):
        if self.laco and self.rng.random() < 0.5:
            return self.laco, (lambda env: env[self.laco].v)
        k = self.rng.randint(0, 4)
        return str(k), (lambda env, k=k: k)

    def bin_int(self, op, ea, eb):
        def ev(env):
            a, b = ea(env).v, eb(env).v
            return Valor(I, wrap(a + b if op == "+" else a - b if op == "-" else a * b))
        return ev

    def div_int(self, ea, eb):
        def ev(env):
            a = ea(env).v
            b = eb(env).v
            d = wrap(wrap(b * b) + 1)
            q = abs(a) // abs(d)
            return Valor(I, wrap(q if (a >= 0) == (d >= 0) else -q))
        return ev

    def pot_int(self, ea, k):
        def ev(env):
            r = 1
            a = ea(env).v
            for _ in range(k):
                r = wrap(r * a)
            return Valor(I, r)
        return ev

    def bin_real(self, op, tipo, ea, eb):
        def ev(env):
            a = converte(ea(env), tipo).v
            b = converte(eb(env), tipo).v
            x = a + b if op == "+" else a - b if op == "-" else a * b
            return Valor(tipo, arred(tipo, x))
        return ev

    def div_real(self, tipo, tipo_b, ea, eb):
        def ev(env):
            vb = eb(env)
            # divisor: (b * b + 1) no tipo de b; o resultado e real.
            if vb.tipo == I:
                d = Valor(I, wrap(wrap(vb.v * vb.v) + 1))
            else:
                q = converte(vb, vb.tipo).v
                sq = arred(vb.tipo, q * q)
                d = Valor(vb.tipo, arred(vb.tipo, sq + 1.0))
            a = converte(ea(env), tipo).v
            dd = converte(d, tipo).v
            return Valor(tipo, arred(tipo, a / dd))
        return ev

    def raiz(self, tipo, ta, ea):
        def ev(env):
            va = ea(env)
            if va.tipo == I:
                sq = Valor(I, wrap(va.v * va.v))
                if sq.v < 0:
                    raise Invalido()
            else:
                sq = Valor(va.tipo, arred(va.tipo, va.v * va.v))
            x = converte(sq, tipo).v
            return Valor(tipo, arred(tipo, math.sqrt(x)))
        return ev

    def quadrado(self, tipo, ea):
        def ev(env):
            a = ea(env).v
            return Valor(tipo, arred(tipo, a * a))
        return ev

    # funcoes do programa (corpos fixos, avaliados aqui)
    def chama_fi(self, a, b):
        a, b = converte(a, I).v, converte(b, I).v
        if a < b:
            return Valor(I, wrap(wrap(a * 3) - b))
        return Valor(I, wrap(a + wrap(b * 2)))

    def chama_ff(self, x):
        x = converte(x, F).v
        return Valor(F, arred(F, arred(F, x * 0.5) + 1.0))

    def booleana(self, prof):
        r = self.rng
        if prof <= 0 or r.random() < 0.2:
            return r.choice([self.literal, self.variavel])(B)
        escolha = r.random()
        if escolha < 0.45:
            ta = r.choice([I, F, D])
            tb = r.choice([I, F, D])
            a, ea = self.num(ta, prof - 1)
            b, eb = self.num(tb, prof - 1)
            op = r.choice(["<", ">"])

            def ev(env):
                t = wider(ta, tb)
                x, y = converte(ea(env), t).v, converte(eb(env), t).v
                return Valor(B, x < y if op == "<" else x > y)
            return f"({a} {op} {b})", ev
        if escolha < 0.6:
            t = r.choice([I, B])
            a, ea = self.num(I, prof - 1) if t == I else self.booleana(prof - 1)
            b, eb = self.num(I, prof - 1) if t == I else self.booleana(prof - 1)
            op = r.choice(["==", "!"])
            return f"({a} {op} {b})", (lambda env: Valor(B, (ea(env).v == eb(env).v) == (op == "==")))
        if escolha < 0.9:
            op = r.choice(["E", "OU", "XOR"])
            a, ea = self.booleana(prof - 1)
            b, eb = self.booleana(prof - 1)

            def ev(env):
                x, y = ea(env).v, eb(env).v
                return Valor(B, (x and y) if op == "E" else (x or y) if op == "OU" else (x != y))
            return f"({a} {op} {b})", ev
        a, ea = self.booleana(prof - 1)
        return f"(NÃO {a})", (lambda env: Valor(B, not ea(env).v))

    def expressao(self, tipo, prof):
        return self.booleana(prof) if tipo == B else self.num(tipo, prof)

    # comandos: geram texto e executam no interpretador
    def comando(self, prof, linhas, recuo):
        r = self.rng
        escolha = r.random()
        if escolha < 0.35:
            tipo = r.choice([I, F, D, B])
            destino = r.choice(self.globais[tipo])
            ftipo = tipo if tipo in (I, B) else r.choice([tipo, I] if tipo == F else [D, F, I])
            texto, ev = self.expressao(ftipo, 3)
            linhas.append(f"{recuo}{destino} receba {texto};")
            valor = ev(self.ambiente)
            self.ambiente[destino] = converte(valor, tipo) if tipo != B else valor
        elif escolha < 0.65:
            tipo = r.choice([I, F, D, B])
            texto, ev = self.expressao(tipo, 3)
            linhas.append(f"{recuo}imprime({texto});")
            valor = ev(self.ambiente)
            self.saida.append(formata(valor.tipo, valor.v))
        elif escolha < 0.75:
            idx_texto, idx = self.indice()
            texto, ev = self.num(I, 2)
            linhas.append(f"{recuo}gv{{{idx_texto}}} receba {texto};")
            self.vetor[idx(self.ambiente)] = ev(self.ambiente).v
        elif escolha < 0.82:
            texto, ev = self.num(I, 2)
            linhas.append(f"{recuo}fv({texto});")
            k = ev(self.ambiente).v
            self.ambiente["g1"] = Valor(I, wrap(self.ambiente["g1"].v + k))
            self.saida.append(str(k))
        elif escolha < 0.92 and prof > 0:
            texto, ev = self.booleana(2)
            linhas.append(f"{recuo}cond ({texto}) edai [")
            corpo_entao = []
            corpo_senao = []
            if ev(self.ambiente).v:
                self.bloco(prof - 1, corpo_entao, recuo + "    ", executa=True)
                self.bloco(prof - 1, corpo_senao, recuo + "    ", executa=False)
            else:
                self.bloco(prof - 1, corpo_entao, recuo + "    ", executa=False)
                self.bloco(prof - 1, corpo_senao, recuo + "    ", executa=True)
            linhas.extend(corpo_entao)
            linhas.append(f"{recuo}]")
            linhas.append(f"{recuo}casocontrario edai [")
            linhas.extend(corpo_senao)
            linhas.append(f"{recuo}]")
        elif prof > 0 and self.laco is None:
            self.laco = "i"
            n = r.randint(1, 5)
            linhas.append(f"{recuo}repete (i receba 0, i < {n}, i receba i + 1) edai [")
            # gera o corpo uma vez e o executa n vezes
            corpo = []
            semente = r.random()
            for k in range(n):
                self.ambiente["i"] = Valor(I, k)
                sub = random.Random(semente)
                antigo = self.rng
                self.rng = sub
                corpo_k = []
                self.bloco(0, corpo_k, recuo + "    ", executa=True)
                self.rng = antigo
                if k == 0:
                    corpo = corpo_k
            linhas.extend(corpo)
            linhas.append(f"{recuo}]")
            self.ambiente["i"] = Valor(I, n)
            self.laco = None
        else:
            linhas.append(f"{recuo}imprime(g0);")
            self.saida.append(str(self.ambiente["g0"].v))

    def bloco(self, prof, linhas, recuo, executa=True):
        if not executa:
            # ramo nao executado: gera comandos sem efeito no interpretador
            salvo = (dict(self.ambiente), list(self.vetor), list(self.saida))
            for _ in range(self.rng.randint(1, 2)):
                self.comando(prof, linhas, recuo)
            self.ambiente, self.vetor, self.saida = dict(salvo[0]), salvo[1], salvo[2]
            return
        for _ in range(self.rng.randint(1, 3)):
            self.comando(prof, linhas, recuo)


def gera_programa(semente):
    rng = random.Random(semente)
    g = Gerador(rng)
    corpo = []
    g.ambiente["i"] = Valor(I, 0)
    for _ in range(rng.randint(6, 14)):
        g.comando(2, corpo, "    ")
    texto = "\n".join([
        "inteira g0, g1, i;",
        "flut gf;",
        "duplocarpado gd;",
        "bool gb;",
        "inteira vet{5} gv;",
        "",
        "outrafuncao inteira fi(inteira a, inteira b) @ [",
        "    cond (a < b) edai [",
        "        respost a * 3 - b;",
        "    ]",
        "    respost a + b * 2;",
        "]",
        "",
        "outrafuncao flut ff(flut x) @ [",
        "    respost x * 0.5 + 1.0;",
        "]",
        "",
        "outrafuncao vazio fv(inteira k) @ [",
        "    g1 receba g1 + k;",
        "    imprime(k);",
        "]",
        "",
        "inteira principal() @ [",
        *corpo,
        "    respost 0;",
        "]",
        "",
    ])
    return texto, "\n".join(g.saida) + ("\n" if g.saida else "")


def executa(comando):
    r = subprocess.run(comando, capture_output=True, text=True, timeout=60)
    return r.returncode, r.stdout + r.stderr


def main():
    quantidade = int(sys.argv[1]) if len(sys.argv) > 1 else 100
    semente0 = int(sys.argv[2]) if len(sys.argv) > 2 else 2026
    pasta = tempfile.mkdtemp(prefix="zl_diferencial_")
    testados = falhas = descartados = 0
    # A biblioteca e compilada uma vez: objeto nativo (clang) e bitcode (lli).
    rt_obj = os.path.join(pasta, "zl_runtime_posix.o")
    rt_bc = os.path.join(pasta, "zl_runtime_posix.bc")
    for passo in ([CLANG, "-O1", "-c", RUNTIME, "-o", rt_obj],
                  [CLANG, "-O1", "-c", "-emit-llvm", RUNTIME, "-o", rt_bc]):
        rc, saida = executa(passo)
        if rc != 0:
            print("nao compilou a biblioteca:\n" + saida)
            return 1
    libm = [] if os.name == "nt" else ["-lm"]
    for n in range(quantidade):
        semente = semente0 * 100000 + n
        try:
            fonte, esperado = gera_programa(semente)
        except (Invalido, OverflowError, ZeroDivisionError):
            descartados += 1
            continue
        base = os.path.join(pasta, f"p{n}")
        with open(base + ".mac", "w", encoding="utf-8", newline="\n") as f:
            f.write(fonte)
        rc, saida = executa([MACARONICA, base + ".mac", "-o", base + ".ll", "-q"])
        problemas = []
        if rc != 0:
            problemas.append("compilador rejeitou:\n" + saida)
        else:
            for passo in ([LLVM_AS, base + ".ll", "-o", base + ".bc"],
                          [OPT, "-passes=verify", "-disable-output", base + ".ll"],
                          [CLANG, "-Wno-override-module", "-O0", base + ".ll", rt_obj, "-o", base + "_O0" + EXE] + libm,
                          [CLANG, "-Wno-override-module", "-O2", base + ".ll", rt_obj, "-o", base + "_O2" + EXE] + libm):
                rc, saida = executa(passo)
                if rc != 0:
                    problemas.append(f"falhou {os.path.basename(passo[0])}:\n{saida}")
                    break
            if not problemas:
                for nome, cmd in (("clang -O0", [base + "_O0" + EXE]),
                                  ("clang -O2", [base + "_O2" + EXE]),
                                  ("lli", [LLI, "--extra-module=" + rt_bc, base + ".ll"])):
                    rc, saida = executa(cmd)
                    if rc != 0 or saida != esperado:
                        problemas.append(f"{nome}: saida {rc}\n--- esperado\n{esperado}--- obtido\n{saida}")
        testados += 1
        if problemas:
            falhas += 1
            print(f"FALHA programa {base}.mac (semente {semente})", flush=True)
            for p in problemas:
                print("  " + p.replace("\n", "\n  ")[:3000])
        elif n % 25 == 0:
            print(f"... {n + 1} programas", flush=True)
    print(f"diferencial: {testados} programas, {falhas} falhas, {descartados} descartados "
          f"(pasta {pasta})")
    return 1 if falhas else 0


if __name__ == "__main__":
    sys.exit(main())
