# Testes próprios (não oficiais)

Os arquivos desta pasta foram escritos por nós, com base no enunciado e nos
slides da disciplina. **Não são exemplos do professor.** Eles cobrem casos
que os exemplos oficiais (`tests/sem_ligacao/` e `tests/com_ligacao/`) não
cobrem. Se o gabarito do professor se comportar diferente em algum detalhe,
vale o gabarito, e estes testes devem ser ajustados.

- `funcionais/`: programas válidos. Cada um tem o `.obj.esperado` calculado
  à mão, e a saída é conferida no `tools/simulador` (casos em
  `simulator_cases` no `run_tests.sh`).
- `erros/`: um programa por tipo de erro. A linha que deve gerar o erro tem
  o comentário `; ESPERADO: TIPO`. Erros do programa inteiro (sem linha)
  são marcados com `; ESPERADO-GLOBAL: TIPO` no cabeçalho. O
  `run_tests.sh` exige que o montador acuse exatamente esses erros (nem
  mais, nem menos), que o `.obj` saia vazio e que o código de saída seja
  diferente de 0.
- `ligacao/`: módulos para os erros do ligador (casos em `linker_cases` no
  `run_tests.sh`).
