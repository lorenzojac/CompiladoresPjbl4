# Ambiente reproduzivel do compilador Macaronica (PjBL 3 e 4).
#
#   docker build -t macaronica .
#   docker run --rm -v "$PWD":/projeto macaronica make test
#   docker run --rm -v "$PWD":/projeto macaronica make bench
#   docker run --rm -v "$PWD":/projeto macaronica make placa PROG=tests/placa/01_laco_vetor.mac
#
# Versoes fixadas pelos pacotes do Debian 13 (trixie): LLVM/Clang/MLIR 19,
# avr-gcc 14 + avr-libc 2.2 (com double de 64 bits), QEMU 10 (qemu-system-avr).
# O desenvolvimento foi feito com LLVM/MLIR 23.1.5 (MSYS2); os passes usados
# em scripts/pipeline.sh existem com o mesmo nome desde o MLIR 18. No LLVM 19
# o executor JIT se chama mlir-cpu-runner; scripts/jit.sh usa o que existir.
# Gravar na placa a partir do conteiner exige repassar a porta USB
# (--device=/dev/ttyACM0); na pratica e mais simples gravar o placa/*.hex do
# lado de fora com o avrdude.
FROM debian:trixie

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential make \
        clang-19 llvm-19 lld-19 libomp-19-dev mlir-19-tools \
        gcc-avr avr-libc binutils-avr avrdude qemu-system-misc \
        python3 python3-matplotlib \
    && rm -rf /var/lib/apt/lists/*

ENV PATH=/usr/lib/llvm-19/bin:$PATH \
    MPLCONFIGDIR=/tmp/matplotlib

WORKDIR /projeto
CMD ["make", "test"]
