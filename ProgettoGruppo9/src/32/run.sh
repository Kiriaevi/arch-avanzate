#!/bin/bash
for f in $(ls *.nasm); do
        nasm -f elf64 -g -DPIC $f;
done;
gcc -msse -g -O0 -fPIC -z noexecstack *.o main.c -o quantpivot -lm

./quantpivot 
