#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "include/quantize.h"
#include <time.h>
#include <math.h>

static const int *arr_ctx;
/**
 * Legge da un file e restituisce un array di float 
 * INPUT: string s, contenente il path da leggere 
 * OUTPUT: puntatore ad un array di float di dimensione D 
 */
float* readFile(char *f) {
  return NULL;
}

/**
 * Calcola un vettore di pivot
 * INPUT: array di float v, intero h 
 * OUTPUT: array di float p di pivot
 */
float* calcoloPivot(float *dataSet, int h, int N, int D) {
  float* pivot = malloc(sizeof(float) * D * h);
  int offset = (int) floorf(N/h);
  int k = 0;

    for (int i = 0; i < N && k < h; i += offset) {
        for (int j = 0; j < D; j++) {
            pivot[k*D + j] = dataSet[i*D + j];
        }
        k++;
    }
  return pivot;
}

/**
 * Esegue una query sull'algoritmo 
 * INPUT: float q[i], singola query 
 * OUTPUT: vettore di K-Nearest Neighbour float
 */
float* executeQuery(float q) {
  return NULL;
} 


int main() {
    int N = 5;   // numero di righe
    int D = 3;   // numero di colonne
    int h = 2;

    srand((unsigned int)time(NULL));

    float *array = malloc(sizeof(float) * N * D);
    if (array == NULL) {
        printf("Errore di allocazione!\n");
        return 1;
    }

    // Riempi dataset
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < D; j++) {
            array[i * D + j] = (float)rand() / (float)RAND_MAX;
        }
    }

    // Stampa dataset
    printf("Dataset (%d x %d):\n", N, D);
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < D; j++) {
            printf("%.3f ", array[i * D + j]);
        }
        printf("\n");
    }

    // ---- TESTA LA FUNZIONE ----
    float *pivots = calcoloPivot(array, h, N, D);

    printf("\nPivot estratti (%d pivot x %d dimensioni):\n", h, D);
    for (int k = 0; k < h; k++) {
        for (int j = 0; j < D; j++) {
            printf("%.3f ", pivots[k * D + j]);
        }
        printf("\n");
    }

    // Libera memoria
    free(array);
    free(pivots);

    return 0;
}
