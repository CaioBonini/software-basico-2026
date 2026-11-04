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

# Erros que o .asm declara esperar, um "LINHA TIPO" por linha:
#   código...  ; ESPERADO: TIPO     -> erro TIPO naquela linha
#   ; ESPERADO-GLOBAL: TIPO         -> erro TIPO sem linha (linha 0)
expected_errors() {
    grep -nE '^[^;].*; ESPERADO: (LEXICO|SINTATICO|SEMANTICO)' "$1" |
        sed -E 's/^([0-9]+):.*; ESPERADO: ([A-Z]+).*/\1 \2/'
    grep -E '^; ESPERADO-GLOBAL: ' "$1" |
        sed -E 's/^; ESPERADO-GLOBAL: ([A-Z]+).*/0 \1/'
}

# Erros que o montador reportou, no mesmo formato "LINHA TIPO".
reported_errors() {
    sed -nE -e 's/^Linha ([0-9]+): erro ([A-Z]+):.*/\1 \2/p' \
        -e 's/^Erro ([A-Z]+):.*/0 \1/p' <<< "$1"
}

# error_case <arquivo.asm>: o montador tem que acusar exatamente os erros
# marcados no .asm, deixar o .obj vazio e sair com código != 0.
error_case() {
    local asm=$1
    local obj=${asm%.asm}.obj
    local out

    rm -f "$obj"
    out=$("$MONTADOR" "$asm" 2>&1)
    local status=$?

    diff_out=$(diff <(expected_errors "$asm" | sort -n) \
                    <(reported_errors "$out" | sort -n))
    if [ -n "$diff_out" ]; then
        fail "$asm: erros diferentes dos marcados (< esperado, > obtido)"
        show_diff
        echo "$out" | sed 's/^/          | /'
    elif [ $status -eq 0 ]; then
        fail "$asm: montador saiu com 0 apesar dos erros"
    elif [ ! -f "$obj" ] || [ -s "$obj" ]; then
        fail "$asm: $obj deveria existir e estar vazio"
    else
        pass "$asm ($(expected_errors "$asm" | wc -l) erro(s) na linha e tipo certos)"
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

# link_error_case <trecho esperado na saída> <mod1.asm> ...: o ligador tem
# que sair com erro, citar o trecho e não deixar executável com conteúdo.
link_error_case() {
    local expected=$1
    shift
    local lig=${1%.asm}.lig
    local name="ligador $* -> \"$expected\""
    local out

    rm -f "$lig"
    out=$("$LIGADOR" "$@" 2>&1)
    local status=$?
    if [ $status -eq 0 ]; then
        fail "$name: saiu com 0"
    elif ! grep -qF -- "$expected" <<< "$out"; then
        fail "$name: mensagem nao encontrada"
        echo "$out" | sed 's/^/          | /'
    elif [ -s "$lig" ]; then
        fail "$name: $lig deveria estar vazio"
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

    # Testes próprios (não oficiais), ver tests/proprios/LEIAME.md.
    local p=tests/proprios/ligacao
    link_error_case "no maximo 3 modulos (recebeu 4)" \
        $p/ok_b.asm $p/ok_c.asm $p/ok_b.asm $p/ok_c.asm
    link_error_case "Erro SEMANTICO: simbolo externo 'Z'" \
        $p/externo_nao_resolvido.asm $p/ok_b.asm
    link_error_case "Erro SEMANTICO: simbolo 'DUP' definido em mais de um modulo" \
        $p/public_dup_a.asm $p/public_dup_b.asm
    link_error_case "Erro SEMANTICO: endereco 65536 fora da memoria" \
        $p/ok_b.asm $p/fora_da_faixa.asm
    link_error_case "modulo_sem_begin.asm: Erro SEMANTICO: modulo sem BEGIN" \
        $p/modulo_sem_begin.asm $p/ok_b.asm
    link_error_case "public_indefinido.asm: Linha 6: erro SEMANTICO: simbolo PUBLIC 'NADA'" \
        $p/public_indefinido.asm $p/ok_b.asm
}

# Casos funcionais: um run_sim por linha.
simulator_cases() {
    run_sim tests/sem_ligacao/old_data.obj "13" "1 0 1 1"
    # Testes próprios (não oficiais), ver tests/proprios/LEIAME.md.
    run_sim tests/proprios/funcionais/space_bloco.obj "3 4" "7 0 99"
    run_sim tests/proprios/funcionais/equ_if.obj "5" "5 10 3"
}

for bin in "$MONTADOR" "$LIGADOR"; do
    if [ ! -x "$bin" ]; then
        echo "erro: $bin não encontrado (rode make antes)" >&2
        exit 2
    fi
done

echo "== Montagem =="
while IFS= read -r asm; do
    if grep -qE '; ESPERADO(-GLOBAL)?: ' "$asm"; then
        error_case "$asm"
    else
        assemble_case "$asm"
    fi
done < <(find tests -name '*.asm' -not -path 'tests/com_ligacao/*' \
              -not -path 'tests/proprios/ligacao/*' | sort)

echo "== Ligacao =="
linker_cases

echo "== Simulador =="
simulator_cases

echo
echo "$passed passaram, $failed falharam"
[ $failed -eq 0 ]
