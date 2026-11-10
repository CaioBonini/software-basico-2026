# Software Básico — 2026.2

Trabalhos da disciplina de Software Básico da UnB, com o professor
Bruno Machiavello, no segundo semestre de 2026.

Feito por Caio Bonini Sampaio (matrícula 251004287).

## Trabalho 1 — Montador e ligador

Um montador e um ligador escritos em C para o assembly didático usado em
aula: um acumulador, 14 instruções e o programa dividido em seção de texto
e seção de dados. O montador lê o arquivo uma vez só e resolve os rótulos
usados antes de serem declarados com backpatching.

Entrega: pasta `Trabalho1_Caio_251004287/` (tem um README próprio com instruções de teste).

Para compilar e rodar os testes, dentro de `Trabalho1_Caio_251004287/`:

```bash
make        # gera ./montador e ./ligador
make test   # testes unitários + comparação com os exemplos
```

Uso:

```bash
./montador programa.asm                 # gera programa.obj
./ligador mod1.asm mod2.asm [mod3.asm]  # liga até 3 módulos
```

## Trabalho 2

Ainda não começou.
