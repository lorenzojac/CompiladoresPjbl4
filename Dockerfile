# Ambiente reproduzivel do compilador Macaronica (PjBL 3 e 4).
#
#   docker build -t macaronica .
#   docker run --rm -v "$PWD":/projeto macaronica make test
#   docker run --rm -v "$PWD":/projeto macaronica make bench
#   docker run --rm -v "$PWD":/projeto -e PICO_BUILD=/projeto/build/pico-docker \
#       macaronica make placa PROG=tests/placa/01_laco_vetor.mac
#   docker run --rm -v "$PWD":/projeto macaronica \
#       make placa PLACA=uno PROG=tests/placa/01_laco_vetor.mac
#
# Versoes fixadas pelos pacotes do Debian 13 (trixie): LLVM/Clang/MLIR 19,
# avr-gcc 14 + avr-libc 2.2 (com double de 64 bits), QEMU 10 (qemu-system-avr),
# arm-none-eabi-gcc 14 + newlib 4.5, CMake 3.31, Ninja 1.12. O Pico SDK e o
# picotool sao os mesmos 2.3.1 do Windows, baixados do GitHub na construcao.
# O desenvolvimento foi feito com LLVM/MLIR 23.1.5 (MSYS2); os passes usados
# em scripts/pipeline.sh existem com o mesmo nome desde o MLIR 18. No LLVM 19
# o executor JIT se chama mlir-cpu-runner; scripts/jit.sh usa o que existir.
# Raspberry Pi Pico 2 W: a pasta build/pico-OFF configurada pelo CMake no
# Windows guarda caminhos do Windows e nao serve aqui; por isso PICO_BUILD
# aponta outra pasta no exemplo acima. Gravar na placa a partir do conteiner
# exige repassar a USB; na pratica e mais simples, do lado de fora, copiar o
# placa/*.uf2 para a unidade RP2350 (modo BOOTSEL) ou gravar o placa/*.hex do
# Uno com o avrdude.
FROM debian:trixie

# PC e MLIR; Arduino Uno (avr-gcc, QEMU); Pico 2 W (gcc ARM, newlib e
# libstdc++ ARM, que o SDK usa em new_delete.cpp, CMake, Ninja); git,
# ca-certificates, libusb e pkg-config para baixar o SDK e compilar o
# picotool; pyserial para scripts/pico.py.
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential make g++ \
        clang-19 llvm-19 lld-19 libomp-19-dev mlir-19-tools \
        gcc-avr avr-libc binutils-avr avrdude qemu-system-misc \
        gcc-arm-none-eabi libnewlib-arm-none-eabi \
        libstdc++-arm-none-eabi-newlib cmake ninja-build \
        git ca-certificates libusb-1.0-0-dev pkg-config \
        python3 python3-matplotlib python3-serial \
    && rm -rf /var/lib/apt/lists/*

# Pico SDK 2.3.1 so com os submodulos usados (~75 MB): tinyusb (porta USB)
# e cyw43-driver (LED da Pico 2 W, ligado ao chip sem fio).
# scripts/pico_sdk.sh acha o SDK pela variavel PICO_SDK_PATH.
RUN git clone --depth 1 --branch 2.3.1 \
        https://github.com/raspberrypi/pico-sdk /opt/pico-sdk \
    && cd /opt/pico-sdk \
    && git submodule update --init --depth 1 lib/tinyusb lib/cyw43-driver

ENV PICO_SDK_PATH=/opt/pico-sdk

# picotool 2.3.1 (o SDK o usa para gerar o .uf2). Instalado em /usr/local,
# com a configuracao do CMake em /usr/local/lib/cmake/picotool, o SDK o acha
# com find_package e nao baixa nem compila outro a cada configuracao.
RUN git clone --depth 1 --branch 2.3.1 \
        https://github.com/raspberrypi/picotool /tmp/picotool \
    && cmake -S /tmp/picotool -B /tmp/picotool/build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DPICO_SDK_PATH=$PICO_SDK_PATH \
    && cmake --build /tmp/picotool/build \
    && cmake --install /tmp/picotool/build \
    && rm -rf /tmp/picotool

ENV PATH=/usr/lib/llvm-19/bin:$PATH \
    MPLCONFIGDIR=/tmp/matplotlib

WORKDIR /projeto
CMD ["make", "test"]
