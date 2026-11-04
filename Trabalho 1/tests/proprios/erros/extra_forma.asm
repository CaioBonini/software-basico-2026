; TESTE PROPRIO (nao oficial). Erros de FORMA da linha que o enunciado nao
; lista, classificados nas etapas 4 e 5 como SINTATICO: EQU sem rotulo,
; secao invalida, EXTERN sem rotulo, '+' sem numero, dois rotulos na mesma
; linha, operando numerico em instrucao, CONST sem numero, SPACE com
; tamanho nao positivo. O gabarito do professor pode classificar diferente.
; Cada linha com o comentario "ESPERADO: TIPO" deve gerar exatamente um
; erro desse tipo, naquela linha; nenhuma outra linha pode gerar erro.
EQU 4           ; ESPERADO: SINTATICO
SECTION CODIGO  ; ESPERADO: SINTATICO
SECTION TEXT
EXTERN          ; ESPERADO: SINTATICO
STORE X +       ; ESPERADO: SINTATICO
A: B: LOAD X    ; ESPERADO: SINTATICO
JMP 5           ; ESPERADO: SINTATICO
STOP
SECTION DATA
X: CONST Y      ; ESPERADO: SINTATICO
Y: SPACE 0      ; ESPERADO: SINTATICO
