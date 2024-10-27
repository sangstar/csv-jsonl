## `csv-jsonl`
A lightweight csv to jsonl converter in C. 

To use, compile the executable:

```bash
gcc -o bin/csv-jsonl main.c
```

Then, call it with the first positional argument being
the relative path to your csv file, and the second
positional argument being the relative path to the
jsonl file this executable will create:

```bash
cd bin && ./csv-jsonl <path-to-csv> <path-to-outputted-jsonl>
```