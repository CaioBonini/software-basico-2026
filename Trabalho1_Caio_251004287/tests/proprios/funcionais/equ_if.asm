; TESTE PROPRIO (nao oficial), escrito com base no enunciado e no slide
; aula6.pdf ("Caracteristicas adicionais dos montadores"). Nao veio do
; professor.
;
; Valida EQU e IF:
;   - EQU declarado antes de qualquer secao, sem ocupar memoria;
;   - IF com valor != 0 inclui a linha seguinte (OUTPUT N sai);
;   - IF com valor 0 pula a linha seguinte (OUTPUT DOBRO nao sai; se
;     saisse, apareceria um 0 extra antes do dobro);
;   - EQU usado como valor de CONST (K vale 3).
;
; Entrada: 5. Saida esperada: 5 10 3
LIGA: EQU 1
DESLIGA: EQU 0
TRES: EQU 3
SECTION TEXT
INPUT N
IF LIGA
OUTPUT N
IF DESLIGA
OUTPUT DOBRO
LOAD N
ADD N
STORE DOBRO
OUTPUT DOBRO
OUTPUT K
STOP
SECTION DATA
N: SPACE
DOBRO: SPACE
K: CONST TRES
