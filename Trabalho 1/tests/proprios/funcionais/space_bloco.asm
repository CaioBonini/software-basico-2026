; TESTE PROPRIO (nao oficial), escrito com base no enunciado e no slide
; aula6.pdf ("Reserva de espaco para variaveis"). Nao veio do professor.
;
; Valida SPACE com argumento > 1:
;   - VET: SPACE 5 reserva 5 palavras consecutivas, todas com 0;
;   - cada posicao do bloco e enderecavel com ROTULO + n;
;   - o tamanho reservado esta certo: DEPOIS, declarado logo depois do
;     bloco, fica em VET + 5 e nao e sobrescrito. Se SPACE 5 reservasse
;     so 1 palavra, VET + 1 cairia em DEPOIS e a 2a saida seria 99, nao 0.
;
; Entrada: 3 4. Saida esperada: 7 0 99
;   7  = VET[0] + VET[4], guardado em VET[2]
;   0  = VET[1], nunca escrito (SPACE grava 0)
;   99 = DEPOIS, intacto
SECTION TEXT
INPUT VET
INPUT VET + 4
LOAD VET
ADD VET + 4
STORE VET + 2
OUTPUT VET + 2
OUTPUT VET + 1
OUTPUT DEPOIS
STOP
SECTION DATA
VET: SPACE 5
DEPOIS: CONST 99
