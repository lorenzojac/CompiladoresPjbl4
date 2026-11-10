#!/bin/sh
# Executa o front-end sobre todos os programas .mac de tests/validos e
# tests/invalidos e confere o codigo de saida:
#   validos/*.mac   -> 0 (PROGRAMA VALIDO)
#   invalidos/*.mac -> 1 (PROGRAMA INVALIDO)
# Uso: sh tests/run_programs.sh ./macaronica
set -u

BIN=${1:-./macaronica}
DIR=$(dirname "$0")
passed=0
failed=0

check() {
    file=$1
    expected=$2
    "$BIN" "$file" > /dev/null 2>&1
    status=$?
    if [ "$status" -eq "$expected" ]; then
        passed=$((passed + 1))
        echo "PASS $file"
    else
        failed=$((failed + 1))
        echo "FAIL $file (saida $status, esperado $expected)"
    fi
}

for f in "$DIR"/validos/*.mac; do
    check "$f" 0
done
for f in "$DIR"/invalidos/*.mac; do
    check "$f" 1
done

echo "$passed programas ok, $failed falhas"
[ "$failed" -eq 0 ]
