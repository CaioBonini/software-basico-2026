; TESTE PROPRIO (nao oficial). Par de minusc_b.asm: ligacao com tudo em
; minusculas (begin, extern, public, end, secao), CRLF e tabs, e simbolos
; externos usados com caixa diferente da declaracao (N/n, Dobra/dobra).
; Este modulo le N, salta para dobra (no outro modulo), e o outro modulo
; volta para volta, que imprime N.
; Entrada: 21. Saida esperada: 42
secao text
prin:	begin
dobra:	extern
n:	extern
public volta
	input N
	jmp Dobra
volta:
	output n
	stop
end
