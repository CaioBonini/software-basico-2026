#!/usr/bin/env bash
# Testes do montador e do ligador.
#
# 1. Roda o montador em cada .asm de tests/ (exceto tests/com_ligacao/, que
#    usa BEGIN/EXTERN e é testado pelo ligador) e compara o .obj gerado com o
#    .obj.esperado ao lado, quando existir (token a token, ignorando espaços).
# 2. Roda o ligador nos casos de linker_cases e compara o executável com a
#    linha de números do .lig.esperado.
# 3. Roda o tools/simulador nos .obj/.lig gerados com uma entrada fixa (casos
#    definidos em simulator_cases, abaixo) e confere a saída.
#
# Uso: tests/run_tests.sh   (a partir de qualquer diretório)
# MONTADOR=caminho e LIGADOR=caminho permitem trocar os binários testados.

cd "$(dirname "$0")/.." || exit 2

MONTADOR=${MONTADOR:-./montador}
LIGADOR=${LIGADOR:-./ligador}
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

# link_case <arquivo .lig.esperado> <mod1.asm> <mod2.asm> [mod3.asm]
# O executável sai com o nome do primeiro módulo; o .esperado pode ter texto
# explicativo, e só a linha formada apenas por números é comparada.
link_case() {
    local expected_file=$1
    shift
    local lig=${1%.asm}.lig
    local name="ligador $*"
    local out

    rm -f "$lig"
    out=$("$LIGADOR" "$@" 2>&1)
    local status=$?
    if [ ! -s "$lig" ]; then
        fail "$name: $lig vazio ou não gerado (ligador saiu com $status)"
        [ -n "$out" ] && echo "$out" | sed 's/^/          /'
        return
    fi
    local expected
    expected=$(grep -E '^-?[0-9]+( -?[0-9]+)*$' "$expected_file")
    if compare_tokens "$(cat "$lig")" "$expected"; then
        pass "$name"
    else
        fail "$name: $lig difere de $expected_file (< obtido, > esperado)"
        show_diff
    fi
}

# link_must_fail <descrição> <args...>: o ligador tem que sair com erro.
link_must_fail() {
    local name="ligador recusa $1"
    shift
    if "$LIGADOR" "$@" >/dev/null 2>&1; then
        fail "$name: saiu com 0"
    else
        pass "$name"
    fi
}

linker_cases() {
    local d=tests/com_ligacao
    link_case $d/mod_a_mod_b.lig.esperado $d/mod_a.asm $d/mod_b.asm
    # Roda já, antes que os casos de falha abaixo sobrescrevam mod_a.lig.
    run_sim $d/mod_a.lig "7" "7 5 12"
    link_must_fail "mais de 3 modulos" \
        $d/mod_a.asm $d/mod_b.asm $d/mod_a.asm $d/mod_b.asm
    link_must_fail "EXTERN nao resolvido (mod_a sozinho com mod1)" \
        $d/mod_a.asm $d/mod1.asm
}

# Casos funcionais: um run_sim por linha.
simulator_cases() {
    run_sim tests/sem_ligacao/old_data.obj "13" "1 0 1 1"
}

for bin in "$MONTADOR" "$LIGADOR"; do
    if [ ! -x "$bin" ]; then
        echo "erro: $bin não encontrado (rode make antes)" >&2
        exit 2
    fi
done

echo "== Montagem =="
while IFS= read -r asm; do
    assemble_case "$asm"
done < <(find tests -name '*.asm' -not -path 'tests/com_ligacao/*' | sort)

echo "== Ligacao =="
linker_cases

echo "== Simulador =="
simulator_cases

echo
echo "$passed passaram, $failed falharam"
[ $failed -eq 0 ]
