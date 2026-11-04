; TESTE PROPRIO (nao oficial). Erro do ligador (Aula7.pdf / secao 2.4.2):
; simbolo EXTERN que nenhum modulo declara como PUBLIC. Ligado com ok_b.asm.
; Esperado: um unico erro SEMANTICO sobre 'Z' (mesmo com dois usos), sem
; numero de linha, e o .lig vazio.
SECTION TEXT
EXT: BEGIN
Z: EXTERN
LOAD Z
STORE Z + 1
STOP
END
