; TESTE PROPRIO (nao oficial). Erro 3 do enunciado (secao 2.2):
; "diretivas e instrucoes invalidas". Classificado como SINTATICO: a palavra
; e um token valido, mas nao e nenhuma instrucao ou diretiva da linguagem.
; Cada linha com o comentario "ESPERADO: TIPO" deve gerar exatamente um
; erro desse tipo, naquela linha; nenhuma outra linha pode gerar erro.
SECTION TEXT
LOAD X
MOV X           ; ESPERADO: SINTATICO
STOP
SECTION DATA
X: CONST 1
V: WORD 3       ; ESPERADO: SINTATICO
