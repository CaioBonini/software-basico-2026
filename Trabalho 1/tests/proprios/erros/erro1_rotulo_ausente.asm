; TESTE PROPRIO (nao oficial). Erro 1 do enunciado (secao 2.2):
; "declaracoes e rotulos ausentes". Classificado como SEMANTICO, porque e
; uma relacao entre uso e declaracao. Sai um erro por uso, na linha do uso.
; Cada linha com o comentario "ESPERADO: TIPO" deve gerar exatamente um
; erro desse tipo, naquela linha; nenhuma outra linha pode gerar erro.
SECTION TEXT
LOAD X
JMP FIM         ; ESPERADO: SEMANTICO
JMPZ FIM        ; ESPERADO: SEMANTICO
STOP
SECTION DATA
X: CONST 1
