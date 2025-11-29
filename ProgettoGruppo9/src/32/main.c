#include <omp.h>
#include <stdlib.h>
#include <xmmintrin.h>
#include <stdio.h>
#include <time.h>
#include "common.h"
#include "quantpivot32.c"

static int N; // Righe dataset
static int D;  // Colonne dataset
static int h;   // numero di pivot
static int x;   // parametro di quantizzazione
static int k;    // numero di vicini
static int silent;

extern float prodScalare(float *v, float *w, int D);

#define FLT_MAX 3.402823466e+38F

//
// ---------------------------------------------------------------
//  PROTOTIPI
// ---------------------------------------------------------------
float *readFile(char *f);
float *executeQuery(float q);

// Aggiornato: firma per buffer esterni
float distanzaApprossimata(float *v, float *w, float *vMinus, float *vPlus, float *wMinus, float *wPlus);

void testQueryingCompleto();

float *querying(float *query, float *pivot, float *dataSet, float *vettoreIndexing);


/*
 * load_data
 * =========
 *
 * Legge da file una matrice di N righe
 * e M colonne e la memorizza in un array lineare in row-major order
 *
 * Codifica del file:
 * primi 4 byte: numero di righe (N) --> numero intero
 * successivi 4 byte: numero di colonne (M) --> numero intero
 * successivi N*M*4 byte: matrix data in row-major order --> numeri floating-point a precisione singola
 */
MATRIX load_data(char* filename, int *n, int *k) {
  FILE* fp;
  int rows, cols, status, i;

  fp = fopen(filename, "rb");

  if (fp == NULL){
    printf("'%s': bad data file name!\n", filename);
    exit(0);
  }

  //TODO: gestiamo il controllo errori? 
  status = fread(&rows, sizeof(int), 1, fp);
  status = fread(&cols, sizeof(int), 1, fp);

  MATRIX data = _mm_malloc(rows*cols*sizeof(type), align);
  status = fread(data, sizeof(type), rows*cols, fp);
  fclose(fp);

  *n = rows;
  *k = cols;

  return data;
}

/*
 * save_data
 * =========
 * * Salva su file un array lineare in row-major order
 * come matrice di N righe e M colonne
 * * Codifica del file:
 * primi 4 byte: numero di righe (N) --> numero intero a 32 bit
 * successivi 4 byte: numero di colonne (M) --> numero intero a 32 bit
 * successivi N*M*4 byte: matrix data in row-major order --> numeri interi o floating-point a precisione singola
 */
void save_data(char* filename, void* X, int n, int k)
{
  FILE* fp = fopen(filename, "wb");
  fwrite(&n, sizeof(int), 1, fp);
  fwrite(&k, sizeof(int), 1, fp);

  // Usa un puntatore a byte per avanzare correttamente
  char* ptr = (char*) X;

  for (int i = 0; i < n; i++) {
    fwrite(ptr, sizeof(type), k, fp);
    ptr += sizeof(type) * k;
  }

  fclose(fp);
}


int main(int argc, char **argv)
{
  // ================= Parametri di ingresso =================
  char* dsfilename = "dataset_2000x256_32.ds2";
  char* queryfilename = "query_2000x256_32.ds2";
  h = 16;
  k = 8;
  x = 64;
  silent = 0;
  // =========================================================

  params* input = malloc(sizeof(params));

  input->DS = load_data(dsfilename, &input->N, &input->D);
  input->Q  = load_data(queryfilename, &input->nq, &input->D);

  // Prima imposta i parametri!!!
  input->h = h;
  input->k = k;
  input->x = x;
  input->silent = silent;

  // Solo ORA puoi allocare id_nn e dist_nn
  input->id_nn   = _mm_malloc(input->nq * input->k * sizeof(int), align);
  input->dist_nn = _mm_malloc(input->nq * input->k * sizeof(type), align);


  N = input->N;
  D = input->D;
  printf("N = %d, D = %d, h = %d, x = %d, k = %d\n", N,D,h,x,k);
  
  clock_t t;
  double elapsed;

  t = clock();
  fit(input);
  t = clock() - t;
  elapsed = ((double)t) / CLOCKS_PER_SEC;
  printf("FIT time = %.5f secs\n", elapsed);

  if(!input->silent)
    printf("FIT time = %.5lf secs\n", elapsed);
  else
    printf("%.3lf\n", elapsed);

  elapsed = 0;

  t = clock();
  predict(input);
  t = clock() - t;
  elapsed = ((double)t) / CLOCKS_PER_SEC;
  printf("FIT time = %.5f secs\n", elapsed);


  if(!input->silent)
    printf("PREDICT time = %.5lf secs\n", elapsed);
  else
    printf("%.3lf\n", elapsed);


  printf("\n=== CONFRONTO PRIME DISTANZE ===\n");

    // 1. Definisci quanti elementi stampare
    int M = 20;                  
    if (M > input->k) M = input->k; // <--- FIX CRUCIALE: Non stampare più di k!
    if (M > input->N) M = input->N;

    float *approx = malloc(M * sizeof(float));
    float *real = malloc(input->N * sizeof(float));

    // Prendi la prima query (Q0)
    // Nota: input->dist_nn è un array piatto. Per la query i-esima, 
    // l'offset è (i * k). Qui stiamo guardando la query 0.
    float *queryVec = &input->Q[0 * D];

    // --- 1) PRENDO LE DISTANZE TROVATE DAL TUO ALGORITMO ---
    for(int j = 0; j < M; j++) {
        approx[j] = input->dist_nn[j]; 
    }

    // --- 2) CALCOLO TUTTE LE DISTANZE REALI (VERIFICA BRUTE FORCE) ---
    // (Questo serve solo per vedere se il tuo approx ha senso)
    for(int i = 0; i < input->N; i++) {
        float *vec = &input->DS[i * D];
        real[i] = dEuclidea(queryVec, vec, D);
    }

    // --- 3) ORDINO LE DISTANZE REALI (Selection Sort parziale sui primi M) ---
    for(int a=0; a<M; a++){
        int min_idx = a;
        for(int b=a+1; b<input->N; b++){
            if(real[b] < real[min_idx]) min_idx = b;
        }
        float tmp = real[a];
        real[a] = real[min_idx];
        real[min_idx] = tmp;
    }

    // --- 4) STAMPA ---
    printf("\nIdx |   Approssimata   |      Reale (Best)\n");
    printf("----------------------------------------\n");

    for(int i=0; i<M; i++){
        // Stampa affiancata:
        // SX: Il vicino che il TUO codice ha scelto
        // DX: Il miglior vicino possibile matematicamente
        printf("%3d | %14.6f | %14.6f\n", i, approx[i], real[i]);
    }

    free(approx);
    free(real);

  // Salva il risultato
  char* outname_id = "out_idnn.ds2";
  char* outname_k = "out_distnn.ds2";
  save_data(outname_id, input->id_nn, input->nq, input->k);
  save_data(outname_k, input->dist_nn, input->nq, input->k);

  if(!input->silent){
    for(int i=0; i<input->nq; i++){
      printf("ID NN Q%3i: ( ", i);
      for(int j=0; j<input->k; j++)
        printf("%i ", input->id_nn[i*input->k + j]);
      printf(")\n");
    }
    for(int i=0; i<input->nq; i++){
      printf("Dist NN Q%3i: ( ", i);
      for(int j=0; j<input->k; j++)
        printf("%f ", input->dist_nn[i*input->k + j]);
      printf(")\n");
    }
  }


  // CALCOLO DISTANZA REALE
  // CALCOLO DISTANZA REALE
  float *realDistances = malloc(input->nq*k*sizeof(float));
  for (int i = 0; i < input->nq; i++)
  {
    float *queryVec = &input->Q[i * D];  // ✅ Prendi la query i-esima
    for(int j = 0; j < k; j++) {
      int idx = input->id_nn[i*k+j];
      float *neighborVec = &input->DS[idx * D];  // ✅ Moltiplica per D
      realDistances[i*k+j] = dEuclidea(queryVec, neighborVec, D);
    }
  }
  printf("\n--- CONFRONTO DISTANZE (nq: %d, k: %d) ---\n", input->nq, k);

  // FIX: Allocazione temporanea per il confronto (opzionale, calcoliamo al volo)
  for (int i = 0; i < input->nq; i++) {
    //printf("Query #%d:\n", i);

    // FIX 1: Il vettore di partenza è la QUERY, non il dataset!
    float *queryVec = &input->Q[i * D]; 

    for (int j = 0; j < k; j++) {
      int idx_neighbor = input->id_nn[i*k+j]; // ID del vicino trovato

      // FIX 2: Calcolo indirizzo vettore vicino. 
      // DEVI moltiplicare idx_neighbor * D per saltare i vettori precedenti!
      float *neighborVec = &input->DS[idx_neighbor * D]; 

      // Calcolo distanza reale (senza ottimizzazioni, per verifica)
      float real = dEuclidea(queryVec, neighborVec, D);

      float stored = input->dist_nn[i*k+j];

      /*printf("  NN %d (ID %d): Real: %10.5f | Stored: %10.5f | Diff: %e\n", 
        j, idx_neighbor, real, stored, real - stored);*/
    }
    //printf("\n"); 
  }

  // Pulizia finale (tutto corretto ora che calcoloPivot usa _mm_malloc)
  free(realDistances);

  // Libera strutture allocate con _mm_malloc
  _mm_free(input->DS);
  _mm_free(input->Q);
  _mm_free(input->P);
  _mm_free(input->index);
  _mm_free(input->id_nn);
  _mm_free(input->dist_nn);

  // Libera la struttura params
  free(input);

  return 0;
}
