# Software Básico — Trabalho 1: Montador e Ligador

- **Aluno:** Caio Bonini Sampaio
- **Matrícula:** 251004287
- **Disciplina:** Software Básico — UnB, 2026.2 (prof. Bruno Machiavello)
- **Linguagem:** C (gcc, `-std=c11 -Wall -Wextra`), sem dependências externas.

## Como compilar

Dentro desta pasta:

```bash
make
```

Isso gera os executáveis `./montador` e `./ligador`.

## Teste rápido (copiar e colar)

Este bloco compila, monta um programa, executa no simulador e liga dois
módulos. Rode dentro desta pasta:

```bash
make && ./montador tests/sem_ligacao/old_data.asm && cat tests/sem_ligacao/old_data.obj && echo "13" | ./tools/simulador tests/sem_ligacao/old_data.obj && ./ligador tests/com_ligacao/mod_a.asm tests/com_ligacao/mod_b.asm && cat tests/com_ligacao/mod_a.lig
```

Resultado esperado:

- `old_data.obj`: `12 29 10 29 4 28 11 30 3 28 11 31 10 29 2 31 11 31 13 31 9 30 29 10 29 7 4 14 2 0 0 0`
- simulador com entrada 13 (binário de 13, dígito a dígito): `1`, `0`, `1`, `1`
- `mod_a.lig`: `12 20 10 11 1 20 11 22 7 12 14 5 13 20 13 11 13 22 5 10 0 0 0`

## Suíte de testes completa

```bash
make test
```

Roda os testes unitários e 43 verificações (comparação dos `.obj`/`.lig`
com os arquivos `.esperado`, execução no simulador, casos de erro e linha de
comando). A última linha deve ser `43 passaram, 0 falharam`.

## Uso

```bash
./montador programa.asm                 # gera programa.obj (mesmo nome, extensão .obj)
./ligador mod1.asm mod2.asm [mod3.asm]  # liga de 2 a 3 módulos; gera mod1.lig
```

- O `.obj` e o `.lig` são uma única linha de inteiros separados por espaço.
- Erros (léxicos, sintáticos e semânticos) são impressos com o número da
  linha e o tipo do erro; nesse caso nenhum arquivo de saída é gerado.
- O montador é de passagem única com backpatching para rótulos usados antes
  de declarados; suporta `SECTION TEXT/DATA` em qualquer ordem, `SPACE`,
  `CONST`, `EQU`, `IF`, e `BEGIN/END/PUBLIC/EXTERN` para ligação.

## Estrutura

```
Makefile
src/        código-fonte (montador, ligador, lexer, tabelas, erros)
tests/      exemplos .asm, saídas .esperado, testes unitários e run_tests.sh
tools/      simulador fornecido pelo professor
```
