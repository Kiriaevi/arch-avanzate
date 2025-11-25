#!/bin/bash
for f in $(ls *.nasm); do
        nasm -f elf32 -DPIC $f;
done;
gcc -msse -m32 -O0 -fPIC -z noexecstack *.o main.c -o quantpivot -lm

./quantpivot
