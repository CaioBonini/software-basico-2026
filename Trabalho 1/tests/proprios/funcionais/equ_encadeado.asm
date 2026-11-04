; TESTE PROPRIO (nao oficial). Casos de EQU/IF alem do basico (slide
; aula6.pdf): IF com numero direto (IF 0 pula, IF 1 inclui), EQU definido a
; partir de outro EQU, e EQU com expressao ROTULO + n.
; Sem entrada. Saida esperada: 2 3 (com um 2 a mais se o IF 0 nao pulasse)
BASE: EQU 2
DOBRO: EQU BASE
TAM: EQU DOBRO + 1
SECTION TEXT
IF 0
OUTPUT A
IF 1
OUTPUT A
OUTPUT B
STOP
SECTION DATA
A: CONST DOBRO
B: CONST TAM
