; TESTE PROPRIO (nao oficial). Erro 6 do enunciado (secao 2.2):
; "instrucoes e diretivas com a quantidade de operandos errada".
; Classificado como SINTATICO.
; Cada linha com o comentario "ESPERADO: TIPO" deve gerar exatamente um
; erro desse tipo, naquela linha; nenhuma outra linha pode gerar erro.
SECTION TEXT
ADD             ; ESPERADO: SINTATICO
LOAD X Y        ; ESPERADO: SINTATICO
COPY X          ; ESPERADO: SINTATICO
STOP X          ; ESPERADO: SINTATICO
STOP
SECTION DATA
X: CONST 1
Y: SPACE 1 2    ; ESPERADO: SINTATICO
Z: CONST        ; ESPERADO: SINTATICO
