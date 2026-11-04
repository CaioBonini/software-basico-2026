; TESTE PROPRIO (nao oficial). SECTION DATA antes de SECTION TEXT (o
; enunciado permite as secoes em qualquer ordem) e CONST negativo, rodados
; no simulador. No .obj os dados vem depois do codigo mesmo assim: NEG fica
; no endereco 13 e R no 14.
; Entrada: 10. Saida esperada: 3 -7
SECTION DATA
NEG: CONST -7
R: SPACE
SECTION TEXT
INPUT R
LOAD R
ADD NEG
STORE R
OUTPUT R
OUTPUT NEG
STOP
