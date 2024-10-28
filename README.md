## `csv-jsonl`
A csv to jsonl converter written in C, in under 360 lines of code.

To use, compile the executable:

```bash
gcc -o bin/csv-jsonl main.c
```

And run it.
```bash
cd bin && ./csv-jsonl <path-to-csv> <path-to-outputted-jsonl>
```

Command line arguments can be listed with `--help`:

```bash
Arguments:
   input_csv                   The path to the csv to be converted
   output_jsonl                The path where the converted jsonl will be created
   -n                  \n      (Optional) Newline escape sequence. Default \n
   --buffer-size       1000    (Optional) Bytes per read
   --buffer-read-size  1000    (Optional) Read size from outer buffer
```
