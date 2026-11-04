; TESTE PROPRIO (nao oficial). Rotulo sem instrucao, regra combinada na
; revisao da etapa 4: rotulo sozinho antes de SECTION, ou no fim do
; arquivo, e erro SINTATICO na linha do rotulo.
; Cada linha com o comentario "ESPERADO: TIPO" deve gerar exatamente um
; erro desse tipo, naquela linha; nenhuma outra linha pode gerar erro.
SECTION TEXT
LOAD X
STOP
FIM:            ; ESPERADO: SINTATICO
SECTION DATA
X: CONST 1
SOBRA:          ; ESPERADO: SINTATICO
