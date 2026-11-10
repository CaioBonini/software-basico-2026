; TESTE PROPRIO (nao oficial). Par de minusc_a.asm (ver la). Usa a forma
; invertida "begin: aux", tambem em minusculas.
secao text
begin: aux
volta:	extern
public dobra
public n
dobra:	load n
	add N
	store n
	jmp VOLTA
end
secao data
n:	space
