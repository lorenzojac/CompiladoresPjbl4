#!/bin/sh
# scripts/pico_sdk.sh - imprime o caminho do Raspberry Pi Pico SDK (D48).
#
# Procura, nesta ordem: $PICO_SDK_PATH; pico-sdk ao lado do projeto;
# ~/pico/pico-sdk; ~/pico-sdk; e, no Windows, %USERPROFILE%\pico\pico-sdk.
# Para instalar (so o necessario para USB e LED, ~75 MB):
#   git clone --depth 1 --branch 2.3.1 https://github.com/raspberrypi/pico-sdk
#   cd pico-sdk && git submodule update --init --depth 1 lib/tinyusb lib/cyw43-driver
set -eu

RAIZ=$(cd "$(dirname "$0")/.." && pwd)
# Pasta do usuario no Windows (o shell do MSYS2 nem sempre exporta
# USERPROFILE; cygpath -H da a pasta C:\Users).
PERFIL=
if command -v cygpath >/dev/null 2>&1; then
    if [ -n "${USERPROFILE:-}" ]; then
        PERFIL=$(cygpath -u "$USERPROFILE")
    else
        PERFIL=$(cygpath -H)/${USERNAME:-${USER:-$(basename "$HOME")}}
    fi
fi

for dir in "${PICO_SDK_PATH:-}" "$RAIZ/../pico-sdk" "$HOME/pico/pico-sdk" \
    "$HOME/pico-sdk" "${PERFIL:+$PERFIL/pico/pico-sdk}"; do
    if [ -n "$dir" ] && [ -f "$dir/pico_sdk_init.cmake" ]; then
        (cd "$dir" && pwd)
        exit 0
    fi
done

echo "Pico SDK nao encontrado: defina PICO_SDK_PATH (ver scripts/pico_sdk.sh)" >&2
exit 1
