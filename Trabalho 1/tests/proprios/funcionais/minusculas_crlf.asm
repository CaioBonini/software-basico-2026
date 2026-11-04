; TESTE PROPRIO (nao oficial). Arquivo "realista", como um aluno poderia
; escrever: tudo em minusculas (secao, equ, if, space, const), quebras de
; linha CRLF (Windows), tabs, comentario colado sem espaco, rotulo sozinho
; numa linha, e rotulos usados com caixa diferente da definicao (vet/Vet/
; VET, fim/Fim, k/K, um/UM, zero/Zero). Valida a regra "nao sensivel a
; caixa" e "ignorar tabulacoes e espacos" de ponta a ponta.
; Entrada: 10. Saida esperada: 6 -4
;   6  = vet[0] + k = 10 + (-4), guardado em vet + 1
;   -4 = k
; Se o "If Zero" nao pulasse a linha, ou o jmp nao chegasse em fim, sairia
; um 10 a mais.
um:	equ 1
zero:	Equ 0
secao text
	input Vet		; le vet[0]
	load vet
	if UM
	add K
	If Zero
	output Vet
	store vet + 1
	jmp Fim
	output Vet
fim:
	output VET + 1;colado
	Output k
	stop
Secao Data
vet:	space 2 ; bloco
K:	const -4
