## `csv-jsonl`
A lightweight csv to jsonl converter in C. 

To use, compile the executable:

```bash
gcc -o bin/csv-jsonl main.c
```

Then, call the executable:

```bash
cd bin && ./csv-jsonl <path-to-csv> <path-to-outputted-jsonl>
```

CLI args can be listed with `--help`:

```bash
Arguments:
   input_csv                   The path to the csv to be converted
   output_jsonl                The path where the converted jsonl will be created
   -n                  \n     (Optional) Newline escape sequence. Default \n
   --buffer-size       1000    (Optional) Bytes per read
   --buffer-read-size  1000    (Optional) Read size from outer buffer
```
