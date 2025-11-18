#!/bin/bash

EXECUTABLE="ProgettoGruppo9/32/main"

if [ -f "$EXECUTABLE" ]; then
    perf record "$EXECUTABLE"
    perf report
else
    echo "Errore: Eseguibile $EXECUTABLE non trovato." >&2
    exit 1
fi
