; TESTE PROPRIO (nao oficial). Erro do ligador (Aula7.pdf / secao 2.4.2):
; "simbolo definido em mais de um modulo". Este modulo e public_dup_b.asm
; declaram PUBLIC DUP. Esperado: erro SEMANTICO sobre 'DUP'.
SECTION TEXT
DUP_A: BEGIN
PUBLIC DUP
DUP: STOP
END
