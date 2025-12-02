#!/bin/bash
for f in $(ls *.nasm); do
        nasm -f elf64 -g -DPIC $f;
done;
gcc -fopenmp -msse -g -O0 -fopenmp -fPIC -z noexecstack *.o main.c -o quantpivot -lm
#gcc -msse -O3 -march=native -fopenmp -fPIC -z noexecstack *.o -ffast-math -fomit-frame-pointer main.c -o testFastMath
#gcc -msse -g -O0 -fsanitize=address -fopenmp -fPIC -z noexecstack *.o -fno-omit-frame-pointer main.c -o quantpivot -lm

./quantpivot 
