#!/usr/bin/env bash
# Testes do montador.
#
# 1. Roda o montador em cada .asm de tests/ (exceto tests/com_ligacao/, que
#    usa BEGIN/EXTERN e é testado pelo ligador) e compara o .obj gerado com o
#    .obj.esperado ao lado, quando existir (token a token, ignorando espaços).
# 2. Roda o tools/simulador nos .obj gerados com uma entrada fixa (casos
#    definidos em simulator_cases, abaixo) e confere a saída.
#
# Uso: tests/run_tests.sh   (a partir de qualquer diretório)
# MONTADOR=caminho permite trocar o binário testado.

cd "$(dirname "$0")/.." || exit 2

MONTADOR=${MONTADOR:-./montador}
SIMULADOR=./tools/simulador
SIM_TIMEOUT=5

passed=0
failed=0

pass() { passed=$((passed + 1)); echo "  OK    $1"; }
fail() { failed=$((failed + 1)); echo "  FALHA $1"; }

# Um token por linha (espaços, tabs e quebras de linha múltiplos viram um só).
tokens() { tr -s ' \t\r\n' '\n' <<< "$1" | sed '/^$/d'; }

# compare_tokens <obtido> <esperado>: falha se diferir, deixando o diff
# token a token em $diff_out para o chamador mostrar com show_diff.
diff_out=
compare_tokens() {
    diff_out=$(diff <(tokens "$1") <(tokens "$2"))
    [ -z "$diff_out" ]
}
show_diff() { echo "$diff_out" | sed 's/^/          /'; }

# assemble_case <arquivo.asm>
assemble_case() {
    local asm=$1
    local obj=${asm%.asm}.obj
    local expected=$obj.esperado
    local out

    rm -f "$obj"
    out=$("$MONTADOR" "$asm" 2>&1)
    local status=$?

    if [ ! -f "$expected" ]; then
        echo "  --    $asm (sem .obj.esperado; montador saiu com $status)"
        return
    fi
    if [ ! -f "$obj" ]; then
        fail "$asm: $obj não foi gerado (montador saiu com $status)"
        [ -n "$out" ] && echo "$out" | sed 's/^/          /'
        return
    fi
    if compare_tokens "$(cat "$obj")" "$(cat "$expected")"; then
        pass "$asm"
    else
        fail "$asm: $obj difere de $expected (< obtido, > esperado)"
        show_diff
    fi
}

# run_sim <arquivo.obj> <entrada> <saída esperada>
# A entrada vai para o stdin do simulador; a saída é comparada em tokens.
run_sim() {
    local obj=$1 input=$2 expected=$3
    local name="simulador $obj (entrada: $input)"

    if [ ! -f "$obj" ]; then
        fail "$name: $obj não existe"
        return
    fi
    local out
    out=$(printf '%s\n' "$input" | timeout "$SIM_TIMEOUT" "$SIMULADOR" "$obj" 2>&1)
    local status=$?
    if [ $status -eq 124 ]; then
        fail "$name: timeout de ${SIM_TIMEOUT}s"
        return
    fi
    if compare_tokens "$out" "$expected"; then
        pass "$name"
    else
        fail "$name: saída difere (< obtido, > esperado)"
        show_diff
    fi
}

# Casos funcionais: um run_sim por linha.
simulator_cases() {
    run_sim tests/sem_ligacao/old_data.obj "13" "1 0 1 1"
}

if [ ! -x "$MONTADOR" ]; then
    echo "erro: $MONTADOR não encontrado (rode make antes)" >&2
    exit 2
fi

echo "== Montagem =="
while IFS= read -r asm; do
    assemble_case "$asm"
done < <(find tests -name '*.asm' -not -path 'tests/com_ligacao/*' | sort)

echo "== Simulador =="
simulator_cases

echo
echo "$passed passaram, $failed falharam"
[ $failed -eq 0 ]
