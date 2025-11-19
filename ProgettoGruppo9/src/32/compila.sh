#!/bin/bash

gcc -z noexecstack -no-pie main.c operazioniVettoriali.c quantize.c -Iinclude -o main 
