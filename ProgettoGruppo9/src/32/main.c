#include "common.h"
#include "quantpivot32.c"
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <xmmintrin.h>

//#define datasetFileName "dataset_2000x256_32.ds2"
//#define queryFileName "query_2000x256_32.ds2"
#define datasetFileName "generated_dataset.ds2"
#define queryFileName "generated_queries.ds2"

static int N; // Righe dataset
static int D; // Colonne dataset
static int h; // numero di pivot
static int x; // parametro di quantizzazione
static int k; // numero di vicini
static int silent;

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
 * successivi N*M*4 byte: matrix data in row-major order --> numeri
 * floating-point a precisione singola
 */
MATRIX load_data(char *filename, int *n, int *k)
{
  FILE *fp;
  int rows, cols, status, i;

  fp = fopen(filename, "rb");

  if (fp == NULL)
  {
    printf("'%s': bad data file name!\n", filename);
    exit(0);
  }

  // TODO: gestiamo il controllo errori?
  status = fread(&rows, sizeof(int), 1, fp);
  status = fread(&cols, sizeof(int), 1, fp);

  MATRIX data = _mm_malloc(rows * cols * sizeof(type), align);
  status = fread(data, sizeof(type), rows * cols, fp);
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
 * successivi N*M*4 byte: matrix data in row-major order --> numeri interi o
 * floating-point a precisione singola
 */
void save_data(char *filename, void *X, int n, int k)
{
  FILE *fp = fopen(filename, "wb");
  fwrite(&n, sizeof(int), 1, fp);
  fwrite(&k, sizeof(int), 1, fp);

  // Usa un puntatore a byte per avanzare correttamente
  char *ptr = (char *)X;

  for (int i = 0; i < n; i++)
  {
    fwrite(ptr, sizeof(type), k, fp);
    ptr += sizeof(type) * k;
  }

  fclose(fp);
}

int main(int argc, char **argv)
{
  // ================= Parametri di ingresso =================
  char *dsfilename = datasetFileName;
  char *queryfilename = queryFileName;
  h = 16;
  k = 8;
  x = 64;
  silent = 1;
  // =========================================================

  params *input = malloc(sizeof(params));

  input->DS = load_data(dsfilename, &input->N, &input->D);
  input->Q = load_data(queryfilename, &input->nq, &input->D);

  input->h = h;
  input->k = k;
  input->x = x;
  input->silent = silent;

  input->id_nn    = _mm_malloc(input->nq * input->k * sizeof(int), align);
  input->dist_nn = _mm_malloc(input->nq * input->k * sizeof(type), align);

  N = input->N;
  D = input->D;
  printf("N = %d, D = %d, h = %d, x = %d, k = %d\n", N, D, h, x, k);

  double t;

  t = omp_get_wtime();
  // =========================================================
  fit(input);
  // =========================================================
  t = omp_get_wtime() - t;

  if (!input->silent)
    printf("FIT time = %.5f secs\n", (double)t);
  else
    printf("%.3f\n", (double)t);

  t = omp_get_wtime();
  // =========================================================
  predict(input);

  // =========================================================
  t = omp_get_wtime() - t;

  if (!input->silent)
    printf("PREDICT time = %.5f secs\n", (double)t);
  else
    printf("%.3f\n", (double)t);

  printf("\n=== CONFRONTO PRIME DISTANZE ===\n");

    int M = 20;                  
    if (M > input->k) M = input->k; 
    if (M > input->N) M = input->N;

  VECTOR approx = malloc(M * sizeof(type));
  VECTOR real = malloc(input->N * sizeof(type));

    VECTOR queryVec = &input->Q[0 * D];

    for(int j = 0; j < M; j++) {
        approx[j] = input->dist_nn[j]; 
    }

    //CALCOLO TUTTE LE DISTANZE REALI
    for(int i = 0; i < input->N; i++) {
        VECTOR vec = &input->DS[i * D];
        real[i] = dEuclidea(queryVec, vec, D);
    }

    for(int a=0; a<M; a++){
        int min_idx = a;
        for(int b=a+1; b<input->N; b++){
            if(real[b] < real[min_idx]) min_idx = b;
        }
        type tmp = real[a];
        real[a] = real[min_idx];
        real[min_idx] = tmp;
    }

    printf("\nIdx |    Approssimata    |       Reale (Best)\n");
    printf("----------------------------------------\n");

    for(int i=0; i<M; i++){
        printf("%3d | %14.6f | %14.6f\n", i, approx[i], real[i]);
    }

    free(approx);
    free(real);

  char* outname_id = "out_idnn.ds2";
  char* outname_k = "out_distnn.ds2";
  save_data(outname_id, input->id_nn, input->nq, input->k);
  save_data(outname_k, input->dist_nn, input->nq, input->k);

  if (!input->silent)
  {
    for (int i = 0; i < input->nq; i++)
    {
      printf("ID NN Q%3i: ( ", i);
      for (int j = 0; j < input->k; j++)
        printf("%i ", input->id_nn[i * input->k + j]);
      printf(")\n");
    }
    for (int i = 0; i < input->nq; i++)
    {
      printf("Dist NN Q%3i: ( ", i);
      for (int j = 0; j < input->k; j++)
        printf("%f ", input->dist_nn[i * input->k + j]);
      printf(")\n");
    }
  }

  // CALCOLO DISTANZA REALE
  VECTOR realDistances = malloc(input->nq*k*sizeof(type));
  for (int i = 0; i < input->nq; i++)
  {
    VECTOR queryVec = &input->Q[i * D];
    for(int j = 0; j < k; j++) {
      int idx = input->id_nn[i*k+j];
      VECTOR neighborVec = &input->DS[idx * D];
      realDistances[i*k+j] = dEuclidea(queryVec, neighborVec, D);
    }
  }
  printf("\n--- CONFRONTO DISTANZE (nq: %d, k: %d) ---\n", input->nq, k);

  for (int i = 0; i < input->nq; i++) {
    VECTOR queryVec = &input->Q[i * D]; 

    for (int j = 0; j < k; j++) {
      int idx_neighbor = input->id_nn[i*k+j];

      VECTOR neighborVec = &input->DS[idx_neighbor * D]; 
      type real = dEuclidea(queryVec, neighborVec, D);
      type stored = input->dist_nn[i*k+j];
    }
    // printf("\n");
  }

  free(realDistances);

  _mm_free(input->DS);
  _mm_free(input->Q);
  _mm_free(input->P);
  _mm_free(input->index);
  _mm_free(input->id_nn);
  _mm_free(input->dist_nn);

  free(input);

  return 0;
}
