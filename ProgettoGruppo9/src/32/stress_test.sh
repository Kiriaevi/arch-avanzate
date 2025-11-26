#!/usr/bin/env bash
set -euo pipefail

# COMPILAZIONE .nasm (se presenti)
shopt -s nullglob
NASM_FILES=( *.nasm )
if [ ${#NASM_FILES[@]} -gt 0 ]; then
  echo "Compilazione file .nasm..."
  for f in "${NASM_FILES[@]}"; do
    echo " nasm -f elf32 -DPIC $f"
    nasm -f elf32 -DPIC "$f"
  done
else
  echo "Nessun .nasm trovato, salto fase nasm."
fi

echo "Compilo mai.c..."
gcc -msse -m32 -O0 -fPIC -z noexecstack *.o main.c -o quantpivot -lm

# --- CONFIGURAZIONI (modificare a piacere) ---
Ns=(2063 10001 20003)     # valori di esempio per N (righe)
Ds=(513 1025)          # dimensioni D (colonne)
hs=(8 16)              # numero di pivot
xs=(9 17 64)              # parametro di quantizzazione
ks=(5 7 10)              # numero di vicini k
# ---------------------------------------------

mkdir -p logs
summary="logs/summary.csv"
echo "N,D,h,x,k,exit_code,errors" > "$summary"

for N in "${Ns[@]}"; do
  for D in "${Ds[@]}"; do
    for h in "${hs[@]}"; do
      for x in "${xs[@]}"; do
        for k in "${ks[@]}"; do
          echo "=== Run: N=$N D=$D h=$h x=$x k=$k ==="
          logfile="logs/run_N${N}_D${D}_h${h}_x${x}_k${k}.log"

          if ./quantpivot "$N" "$D" "$h" "$x" "$k" > "$logfile" 2>&1; then
            exitcode=0
          else
            exitcode=$?
          fi

          errors=$(grep -Eo "Trovati [0-9]+ errori!" "$logfile" | grep -Eo "[0-9]+" || echo 0)

          echo "${N},${D},${h},${x},${k},${exitcode},${errors}" >> "$summary"
        done
      done
    done
  done
done

echo "Tutti i test completati. Summary: $summary"
echo "Log per run nella cartella logs/"
