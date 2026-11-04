; TESTE PROPRIO (nao oficial). Erro 2 do enunciado (secao 2.2):
; "declaracoes e rotulos repetidos". Classificado como SEMANTICO. O erro
; sai na segunda declaracao, tanto no texto quanto nos dados.
; Cada linha com o comentario "ESPERADO: TIPO" deve gerar exatamente um
; erro desse tipo, naquela linha; nenhuma outra linha pode gerar erro.
SECTION TEXT
L: LOAD X
JMP L
L: STOP         ; ESPERADO: SEMANTICO
SECTION DATA
X: CONST 1
X: SPACE        ; ESPERADO: SEMANTICO
