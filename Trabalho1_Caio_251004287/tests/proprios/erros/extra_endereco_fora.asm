; TESTE PROPRIO (nao oficial). Endereco fora da memoria da maquina (2^16
; palavras, enderecos 0 a 65535). O enunciado nao lista esse erro; o slide
; Aula7.pdf o cita para o ligador, e decidimos checar tambem no montador
; avulso (mesma funcao do ligador). Classificado como SEMANTICO, na linha
; da instrucao. Texto = 8 palavras (contando o OUTPUT N da 2a secao TEXT),
; entao X fica no endereco 8:
;   X + 65527 = 65535 (limite, cabe); X + 65528 = 65536 (passa);
;   X + -9 = -1 (negativo); N = 70000 vem de EQU e tambem e endereco.
; Cada linha com o comentario "ESPERADO: TIPO" deve gerar exatamente um
; erro desse tipo, naquela linha; nenhuma outra linha pode gerar erro.
N: EQU 70000
SECTION TEXT
LOAD X + 65527
STORE X + 65528 ; ESPERADO: SEMANTICO
ADD X + -9      ; ESPERADO: SEMANTICO
SECTION DATA
X: SPACE
SECTION TEXT
OUTPUT N        ; ESPERADO: SEMANTICO
