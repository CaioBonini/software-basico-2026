; TESTE PROPRIO (nao oficial). Arquivo salvo com BOM UTF-8 (bytes EF BB BF
; no inicio), como fazem alguns editores do Windows. O BOM e invisivel no
; editor e nao e conteudo: o montador tem que ignora-lo. Antes da correcao,
; o BOM grudava na primeira linha e o arquivo inteiro dava erro.
; Entrada: 9. Saida esperada: 9
SECTION TEXT
INPUT N
OUTPUT N
STOP
SECTION DATA
N: SPACE
