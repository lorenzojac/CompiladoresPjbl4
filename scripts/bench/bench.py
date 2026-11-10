"""scripts/bench/bench.py - estudo experimental de otimizacao (PjBL 4, Parte C).

Uso: python3 scripts/bench/bench.py [--rapido]     (ou: make bench)

1. Descreve a maquina (CPU, nucleos, extensoes SIMD).
2. Para N = 10^6 e 10^7, gera nucleos.mac com a DSL vetorial e o compilador
   do grupo produz os 4 nucleos MLIR (elemento a elemento, soma inteira,
   soma flut, produto escalar flut).
3. Constroi cada configuracao (pipeline.sh + clang) medindo o tempo de
   compilacao, e executa scripts/bench/harness.c medindo o tempo de execucao.
   Cada medida e a MEDIANA de 5 repeticoes.
      C0          pipeline sem vetorizar, clang -O0           (linha de base)
      C1          pipeline sem vetorizar, clang -O2            (LLVM vetoriza)
      C1-fastmath C1 com nucleos gerados com --fastmath (fadd fast no IR)
      C2          affine-super-vectorize, clang -O2 sem vetorizacao do LLVM
      C2+C1       affine-super-vectorize e vetorizacao do LLVM
      C3-tT       affine-parallelize + OpenMP, T threads (1, 2, 4, ... nucleos)
4. Guarda as evidencias: relatorios -Rpass do vetorizador do LLVM, numero de
   operacoes vetoriais no IR de cada nucleo, opt -O2 com e sem target triple.
5. Escreve build/bench/resultados.csv, docs/bench/resultados.md e graficos
   docs/bench/*.png (matplotlib).
"""
import array
import csv
import math
import os
import platform
import re
import shutil
import statistics
import subprocess
import sys
import time

RAIZ = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
BUILD = os.path.join(RAIZ, "build", "bench")
DOCS = os.path.join(RAIZ, "docs", "bench")
EXE = ".exe" if os.name == "nt" or platform.system().startswith(("MINGW", "MSYS")) else ""
CLANG = os.environ.get("CLANG", "clang")
OPT = os.environ.get("OPT", "opt")
MACARONICA = os.path.join(RAIZ, "macaronica")
RAPIDO = "--rapido" in sys.argv
AMOSTRAS = 3 if RAPIDO else 5
TAMANHOS = [1_000_000] if RAPIDO else [1_000_000, 10_000_000]
REPETICOES = {1_000_000: 40, 10_000_000: 8}
NUCLEOS = ["soma_elemento", "somavet_inteira", "somavet_flut", "prodesc_flut"]

PROGRAMA = """flut vet{{{n}}} a, b, c;
inteira vet{{{n}}} ia;

inteira principal() @ [
    c receba a + b;
    imprime(somavet(ia));
    imprime(somavet(a));
    imprime(prodesc(a, b));
    respost 0;
]
"""


def roda(comando, ambiente=None, entrada=None):
    resultado = subprocess.run(comando, capture_output=True, text=True, env=ambiente, input=entrada)
    if resultado.returncode != 0:
        raise SystemExit(f"falhou: {' '.join(comando)}\n{resultado.stdout}{resultado.stderr}")
    return resultado


def descreve_maquina():
    cpu = platform.processor() or "desconhecida"
    if os.name == "nt":
        try:
            saida = subprocess.run(
                ["powershell.exe", "-NoProfile", "-Command",
                 "(Get-CimInstance Win32_Processor).Name"],
                capture_output=True, text=True).stdout.strip()
            cpu = saida or cpu
        except OSError:
            pass
    elif os.path.exists("/proc/cpuinfo"):
        for linha in open("/proc/cpuinfo", encoding="utf-8"):
            if linha.startswith("model name"):
                cpu = linha.split(":", 1)[1].strip()
                break
    macros = subprocess.run([CLANG, "-march=native", "-dM", "-E", "-x", "c", "-"],
                            input="", capture_output=True, text=True).stdout
    simd = sorted(set(re.findall(r"__(AVX512F|AVX2|AVX|FMA|SSE4_2|ARM_NEON)__", macros)))
    versao = roda([CLANG, "--version"]).stdout.splitlines()[0]
    return {
        "cpu": cpu,
        "logicos": os.cpu_count(),
        "simd": ", ".join(simd),
        "sistema": f"{platform.system()} {platform.release()}",
        "clang": versao,
    }


def configuracoes():
    base = ["-O2", "-march=native"]
    lista = [
        ("C0", "C0", ["-O0"], None),
        ("C1", "C0", base, None),
        ("C1-fastmath", "C0", base, None),
        ("C2", "C2", base + ["-fno-vectorize", "-fno-slp-vectorize"], None),
        ("C2+C1", "C2", base, None),
    ]
    logicos = os.cpu_count() or 1
    threads = sorted({t for t in (1, 2, 4, 8, 14, logicos) if t <= logicos})
    for t in threads:
        lista.append((f"C3-t{t}", "C3", base + ["-fno-vectorize", "-fno-slp-vectorize", "-fopenmp"], t))
    return lista


def conta_vetores(ir):
    """Operacoes com tipos vetoriais LLVM (<N x T>) por funcao zl_nucleo_k."""
    contagem = {}
    atual = None
    for linha in ir.splitlines():
        m = re.match(r"define .*@(zl_nucleo_\d+)\(", linha)
        if m:
            atual = m.group(1)
            contagem[atual] = 0
        elif linha.startswith("}"):
            atual = None
        elif atual and re.search(r"<\d+ x (i32|float)>", linha):
            contagem[atual] += 1
    return contagem


def main():
    os.makedirs(BUILD, exist_ok=True)
    os.makedirs(DOCS, exist_ok=True)
    maquina = descreve_maquina()
    print("Maquina:", maquina)
    linhas = []
    compilacoes = []
    evidencias = {}

    for n in TAMANHOS:
        pasta = os.path.join(BUILD, f"n{n}")
        os.makedirs(pasta, exist_ok=True)
        fonte = os.path.join(pasta, "nucleos.mac")
        with open(fonte, "w", encoding="utf-8", newline="\n") as f:
            f.write(PROGRAMA.format(n=n))
        roda([MACARONICA, fonte, "-o", os.path.join(pasta, "nucleos.ll"), "-q"])
        # Mesmos nucleos com permissao de reassociar (fastmath<fast>).
        roda([MACARONICA, fonte, "-o", os.path.join(pasta, "nucleos_fast.ll"), "-q", "--fastmath"])
        mlir_normal = os.path.join(pasta, "nucleos.mlir")
        harness_obj = os.path.join(pasta, "harness.o")
        roda([CLANG, "-O2", f"-DN={n}", "-c", os.path.join(RAIZ, "scripts", "bench", "harness.c"),
              "-o", harness_obj])

        for nome, mlir_config, flags, threads in configuracoes():
            mlir = os.path.join(pasta, "nucleos_fast.mlir") if nome == "C1-fastmath" else mlir_normal
            nucleo_ll = os.path.join(pasta, f"nucleos_{nome}.ll")
            executavel = os.path.join(pasta, f"bench_{nome}{EXE}")
            tempos_compilacao = []
            for _ in range(AMOSTRAS):
                inicio = time.perf_counter()
                roda(["sh", os.path.join(RAIZ, "scripts", "pipeline.sh"), mlir, nucleo_ll, mlir_config])
                roda([CLANG, "-Wno-override-module", *flags, nucleo_ll, harness_obj, "-o", executavel])
                tempos_compilacao.append(time.perf_counter() - inicio)
            compilacoes.append({"n": n, "config": nome, "compilacao_s": statistics.median(tempos_compilacao)})

            if n == TAMANHOS[0] and threads in (None, 1):
                otimizado = roda([CLANG, "-Wno-override-module", *[f for f in flags if f != "-fopenmp"],
                                  "-S", "-emit-llvm", nucleo_ll, "-o", "-"]).stdout
                evidencias[nome] = conta_vetores(otimizado)
                with open(os.path.join(DOCS, f"ir_{nome}.ll"), "w", encoding="utf-8", newline="\n") as f:
                    f.write(otimizado)
                if nome in ("C1", "C1-fastmath"):
                    rpass = roda([CLANG, "-Wno-override-module", *flags, "-c", nucleo_ll, "-o",
                                  os.path.join(pasta, "rpass.o"), "-Rpass=loop-vectorize",
                                  "-Rpass-missed=loop-vectorize", "-Rpass-analysis=loop-vectorize"]).stderr
                    with open(os.path.join(DOCS, f"rpass_{nome}.txt"), "w", encoding="utf-8", newline="\n") as f:
                        f.write(rpass.replace(pasta, "build/bench/n" + str(n)))

            ambiente = dict(os.environ)
            if threads is not None:
                ambiente["OMP_NUM_THREADS"] = str(threads)
            medidas = {k: [] for k in NUCLEOS}
            resultados = {}
            for _ in range(AMOSTRAS):
                saida = roda([executavel, str(REPETICOES[n])], ambiente=ambiente).stdout
                for linha in saida.strip().splitlines():
                    nucleo, segundos, valor = linha.split(",")
                    medidas[nucleo].append(float(segundos))
                    resultados[nucleo] = valor
            for nucleo in NUCLEOS:
                linhas.append({
                    "n": n, "config": nome, "threads": threads or "",
                    "nucleo": nucleo, "segundos": statistics.median(medidas[nucleo]),
                    "resultado": resultados[nucleo],
                })
            print(f"N={n} {nome}: " + ", ".join(
                f"{k}={statistics.median(v) * 1e3:.3f} ms" for k, v in medidas.items()))

    # opt -O2 sem e com target triple (Aula 12): sem triple nao ha vetorizacao.
    nucleo_c1 = os.path.join(BUILD, f"n{TAMANHOS[0]}", "nucleos_C1.ll")
    sem_triple = roda([OPT, "-O2", "-S", nucleo_c1, "-o", "-"]).stdout
    triple = roda([CLANG, "-print-target-triple"]).stdout.strip()
    com_triple = roda([OPT, "-O2", f"-mtriple={triple}", "-mcpu=native", "-S", nucleo_c1, "-o", "-"]).stdout
    triple_evidencia = (sum(conta_vetores(sem_triple).values()), sum(conta_vetores(com_triple).values()), triple)

    with open(os.path.join(BUILD, "resultados.csv"), "w", newline="", encoding="utf-8") as f:
        escritor = csv.DictWriter(f, fieldnames=["n", "config", "threads", "nucleo", "segundos", "resultado"])
        escritor.writeheader()
        escritor.writerows(linhas)
    shutil.copy(os.path.join(BUILD, "resultados.csv"), os.path.join(DOCS, "resultados.csv"))
    escreve_markdown(maquina, linhas, compilacoes, evidencias, triple_evidencia)
    desenha_graficos(linhas)
    print("Resultados em docs/bench/resultados.md")


def f32(x):
    """Arredonda para float de 32 bits (como o C faz em cada operacao)."""
    return array.array("f", [x])[0]


def referencia_exata(n):
    """Somas exatas (math.fsum) dos mesmos dados float do harness.c."""
    a = []
    b = []
    for i in range(n):
        x = f32(f32((i * 7919) % 1000) / f32(7.0))
        a.append(f32(x + (1e4 if i % 3 == 0 else 0.0)))
        b.append(f32(f32((i * 104729) % 997) / f32(13.0)))
    return math.fsum(a), math.fsum(x * y for x, y in zip(a, b))


def escreve_markdown(maquina, linhas, compilacoes, evidencias, triple_evidencia):
    md = ["# Resultados do estudo de otimizacao (gerado por scripts/bench/bench.py)", ""]
    md.append(f"- CPU: {maquina['cpu']} ({maquina['logicos']} processadores logicos)")
    md.append(f"- Extensoes SIMD: {maquina['simd']}")
    md.append(f"- Sistema: {maquina['sistema']}; {maquina['clang']}")
    md.append(f"- Cada valor: mediana de {AMOSTRAS} execucoes; tempo por chamada do nucleo.")
    md.append("")
    for n in sorted({l["n"] for l in linhas}):
        md.append(f"## N = {n:,}".replace(",", "."))
        md.append("")
        md.append("| Configuracao | " + " | ".join(NUCLEOS) + " | compilacao (s) |")
        md.append("|---|" + "---:|" * (len(NUCLEOS) + 1))
        base = {l["nucleo"]: l["segundos"] for l in linhas if l["n"] == n and l["config"] == "C0"}
        for config in dict.fromkeys(l["config"] for l in linhas if l["n"] == n):
            celulas = []
            for nucleo in NUCLEOS:
                s = next(l["segundos"] for l in linhas
                         if l["n"] == n and l["config"] == config and l["nucleo"] == nucleo)
                celulas.append(f"{s * 1e3:.3f} ms ({base[nucleo] / s:.1f}x)")
            comp = next(c["compilacao_s"] for c in compilacoes if c["n"] == n and c["config"] == config)
            md.append(f"| {config} | " + " | ".join(celulas) + f" | {comp:.2f} |")
        md.append("")
        md.append("Resultado numerico de cada nucleo (para comparar a ordem das somas):")
        md.append("")
        md.append("| Configuracao | " + " | ".join(NUCLEOS) + " |")
        md.append("|---|" + "---:|" * len(NUCLEOS))
        for config in dict.fromkeys(l["config"] for l in linhas if l["n"] == n):
            valores = [next(l["resultado"] for l in linhas
                            if l["n"] == n and l["config"] == config and l["nucleo"] == k) for k in NUCLEOS]
            md.append(f"| {config} | " + " | ".join(valores) + " |")
        soma, produto = referencia_exata(n)
        md.append(f"| **exato** (math.fsum) | | | {soma:.6f} | {produto:.6f} |")
        md.append("")
    md.append("## Instrucoes vetoriais no IR otimizado de cada nucleo")
    md.append("")
    md.append("Numero de instrucoes com tipo `<N x i32>`/`<N x float>` (N = 10^6):")
    md.append("")
    nomes_nucleo = {"zl_nucleo_1": "soma_elemento", "zl_nucleo_2": "somavet_inteira",
                    "zl_nucleo_3": "somavet_flut", "zl_nucleo_4": "prodesc_flut"}
    md.append("| Configuracao | " + " | ".join(nomes_nucleo.values()) + " |")
    md.append("|---|" + "---:|" * len(nomes_nucleo))
    for config, contagem in evidencias.items():
        md.append(f"| {config} | " + " | ".join(str(contagem.get(k, 0)) for k in nomes_nucleo) + " |")
    md.append("")
    md.append(f"`opt -O2` sobre o IR do mlir-translate: {triple_evidencia[0]} instrucoes vetoriais "
              f"sem target triple; {triple_evidencia[1]} com `-mtriple={triple_evidencia[2]} -mcpu=native`.")
    md.append("")
    with open(os.path.join(DOCS, "resultados.md"), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(md) + "\n")


def desenha_graficos(linhas):
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        print("matplotlib ausente: graficos nao gerados")
        return
    for n in sorted({l["n"] for l in linhas}):
        configs = [c for c in dict.fromkeys(l["config"] for l in linhas if l["n"] == n)
                   if not c.startswith("C3") or c in ("C3-t1",)]
        configs += [c for c in dict.fromkeys(l["config"] for l in linhas if l["n"] == n)
                    if c.startswith("C3") and c != "C3-t1"][-1:]
        figura, eixo = plt.subplots(figsize=(10, 5))
        largura = 0.8 / len(configs)
        for i, config in enumerate(configs):
            valores = [next(l["segundos"] for l in linhas if l["n"] == n and l["config"] == config
                            and l["nucleo"] == k) * 1e3 for k in NUCLEOS]
            eixo.bar([x + i * largura for x in range(len(NUCLEOS))], valores, largura, label=config)
        eixo.set_xticks([x + 0.4 - largura / 2 for x in range(len(NUCLEOS))])
        eixo.set_xticklabels(NUCLEOS)
        eixo.set_yscale("log")
        eixo.set_ylabel("tempo por chamada (ms, escala log)")
        eixo.set_title(f"Nucleos da DSL vetorial, N = {n:,}".replace(",", "."))
        eixo.legend()
        figura.tight_layout()
        figura.savefig(os.path.join(DOCS, f"tempos_n{n}.png"), dpi=120)
        plt.close(figura)

        figura, eixo = plt.subplots(figsize=(7, 5))
        threads_cfg = [(int(l["threads"]), l) for l in linhas
                       if l["n"] == n and l["config"].startswith("C3")]
        for nucleo in NUCLEOS:
            pontos = sorted((t, l["segundos"]) for t, l in threads_cfg if l["nucleo"] == nucleo)
            if not pontos:
                continue
            base = pontos[0][1]
            eixo.plot([t for t, _ in pontos], [base / s for _, s in pontos], marker="o", label=nucleo)
        maximo = max((t for t, _ in threads_cfg), default=1)
        eixo.plot([1, maximo], [1, maximo], linestyle="--", color="gray", label="ideal")
        eixo.set_xlabel("threads (OMP_NUM_THREADS)")
        eixo.set_ylabel("speedup em relacao a 1 thread")
        eixo.set_title(f"C3 (OpenMP), N = {n:,}".replace(",", "."))
        eixo.legend()
        figura.tight_layout()
        figura.savefig(os.path.join(DOCS, f"speedup_n{n}.png"), dpi=120)
        plt.close(figura)


if __name__ == "__main__":
    main()
