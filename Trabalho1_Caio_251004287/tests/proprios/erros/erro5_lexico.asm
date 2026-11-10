; TESTE PROPRIO (nao oficial). Erro 5 do enunciado (secao 2.2): "erros
; lexicos". Classificado como LEXICO: token malformado. Cada linha tem um
; so erro, sem erro em cascata (ex.: COPY X,Y nao acusa tambem a
; quantidade de operandos). "123: STOP" (so digitos) e LEXICO como "1ROT:".
; Cada linha com o comentario "ESPERADO: TIPO" deve gerar exatamente um
; erro desse tipo, naquela linha; nenhuma outra linha pode gerar erro.
SECTION TEXT
LOAD X
1ROT: STOP      ; ESPERADO: LEXICO
ADD X$          ; ESPERADO: LEXICO
123: STOP       ; ESPERADO: LEXICO
COPY X,Y        ; ESPERADO: LEXICO
STOP
SECTION DATA
X: CONST 1
Y: SPACE
Z: CONST 0X1F   ; ESPERADO: LEXICO
ESTE_NOME_TEM_MAIS_DE_TRINTA_CHARS: SPACE   ; ESPERADO: LEXICO
