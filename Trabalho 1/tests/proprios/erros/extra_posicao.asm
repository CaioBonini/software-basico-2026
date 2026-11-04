; TESTE PROPRIO (nao oficial). Erros de POSICAO que o enunciado nao lista,
; classificados na etapa 5 como SEMANTICO (dependem do contexto do
; programa, nao da forma da linha): linha antes de qualquer secao, CONST
; fora de DATA, instrucao fora de TEXT, IF fora de TEXT, EQU depois de uma
; secao. O gabarito do professor pode classificar diferente.
; Cada linha com o comentario "ESPERADO: TIPO" deve gerar exatamente um
; erro desse tipo, naquela linha; nenhuma outra linha pode gerar erro.
UM: EQU 1
STOP            ; ESPERADO: SEMANTICO
SECTION TEXT
LOAD X
A: CONST 5      ; ESPERADO: SEMANTICO
STOP
SECTION DATA
X: CONST 1
ADD X           ; ESPERADO: SEMANTICO
IF UM           ; ESPERADO: SEMANTICO
DOIS: EQU 2     ; ESPERADO: SEMANTICO
