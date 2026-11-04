; TESTE PROPRIO (nao oficial). Erro do ligador (secao 2.4.2): endereco
; final fora da memoria da maquina (0 a 65535). Neste modulo X fica no
; endereco local 3, e X + 65532 = 65535 ainda cabe, entao o montador aceita.
; Ligado DEPOIS de ok_b.asm (1 palavra), o fator de correcao e 1 e o
; endereco final vira 65536: so o ligador consegue acusar.
SECTION TEXT
LONGE: BEGIN
LOAD X + 65532
STOP
END
SECTION DATA
X: SPACE
