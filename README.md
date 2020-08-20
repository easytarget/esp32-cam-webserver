# Converted original files:
## Might be outdated vs. the Expressif ezxample, these date from when I forked their code. 2019'ish

Both `.h` files are adapted to drop the `PROGMEM` reference and use character arrays.

`dumper.c`, when compiled, dumps the indexes out in gzipped format. I then ran `gunzip *.gz` in the folder to get the plain text files.
