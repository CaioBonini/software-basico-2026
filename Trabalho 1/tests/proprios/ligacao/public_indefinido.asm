; TESTE PROPRIO (nao oficial). Erro do modo ligador do montador (etapa 7):
; PUBLIC de um simbolo que nao e definido no modulo. SEMANTICO, na linha do
; PUBLIC. Ligado com ok_b.asm.
SECTION TEXT
PUB: BEGIN
PUBLIC NADA
STOP
END
