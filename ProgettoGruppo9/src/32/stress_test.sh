#!/bin/bash

OUTPUT_BIN="main"
COMPILE_CMD="gcc -z noexecstack -no-pie main.c operazioniVettoriali.c quantize.c -Iinclude -o main "
TEST_CASES=(
  "100 10 5 4 5"
  "500 20 10 8 10"
  "1000 50 20 10 20"
  "2000 64 30 16 50"
  "5000 128 40 32 50"
)

print_header() {
  printf "\n"
  printf "=================================================================\n"
  printf "  SYSTEM PROFILING AND TESTING SUITE\n"
  printf "=================================================================\n"
}

print_separator() {
  printf "-----------------------------------------------------------------\n"
}

print_header
printf "Phase 1: Build Process\n"
print_separator

printf "Executing: %s\n" "$COMPILE_CMD"
$COMPILE_CMD

if [ $? -ne 0 ]; then
  printf "\n[FATAL ERROR] Compilation failed. Aborting process.\n"
  exit 1
else
  printf "Build status: SUCCESS\n"
fi

printf "\nPhase 2: Execution Profiling\n"
print_separator

printf "%-5s | %-6s | %-6s | %-6s | %-6s | %-6s | %-10s\n" "ID" "N" "D" "h" "x" "k" "RESULT"
print_separator

test_id=1
for config in "${TEST_CASES[@]}"; do
  read -r n_val d_val h_val x_val k_val <<<"$config"

  ./$OUTPUT_BIN $n_val $d_val $h_val $x_val $k_val >/dev/null 2>&1

  exit_code=$?

  if [ $exit_code -eq 0 ]; then
    status="PASS"
  else
    status="FAIL ($exit_code)"
  fi

  printf "%-5d | %-6d | %-6d | %-6d | %-6d | %-6d | %-10s\n" \
    "$test_id" "$n_val" "$d_val" "$h_val" "$x_val" "$k_val" "$status"

  if [ $exit_code -ne 0 ]; then
    print_separator
    printf "[CRITICAL] Test case #%d failed. Halting execution.\n" "$test_id"
    exit 1
  fi

  ((test_id++))
done

print_separator
printf "Summary: All %d configurations executed successfully.\n" "$((test_id - 1))"
printf "=================================================================\n\n"
