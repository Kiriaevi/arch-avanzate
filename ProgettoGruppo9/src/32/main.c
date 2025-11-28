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
*
* 	load_data
* 	=========
*
*	Legge da file una matrice di N righe
* 	e M colonne e la memorizza in un array lineare in row-major order
*
* 	Codifica del file:
* 	primi 4 byte: numero di righe (N) --> numero intero
* 	successivi 4 byte: numero di colonne (M) --> numero intero
* 	successivi N*M*4 byte: matrix data in row-major order --> numeri floating-point a precisione singola
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
* 	save_data
* 	=========
* 
*	Salva su file un array lineare in row-major order
*	come matrice di N righe e M colonne
* 
* 	Codifica del file:
* 	primi 4 byte: numero di righe (N) --> numero intero a 32 bit
* 	successivi 4 byte: numero di colonne (M) --> numero intero a 32 bit
* 	successivi N*M*4 byte: matrix data in row-major order --> numeri interi o floating-point a precisione singola
*/
void save_data(char* filename, void* X, int n, int k) 
{
	FILE* fp;
	int i;
	fp = fopen(filename, "wb");
	if(X != NULL){
		fwrite(&n, 4, 1, fp);
		fwrite(&k, 4, 1, fp);
		for (i = 0; i < n; i++) {
			fwrite(X, sizeof(type), k, fp);
			//printf("%i %i\n", ((int*)X)[0], ((int*)X)[1]);
			X += sizeof(type)*k;
		}
	}
	else{
		int x = 0;
		fwrite(&x, 4, 1, fp);
		fwrite(&x, 4, 1, fp);
	}
	fclose(fp);
}


//void testQueryingCompleto()
//{
//  printf("\n===== TEST querying COMPLETO =====\n");
//
//  // 1. Genera dataset casuale
//  float *dataset = malloc(N * D *  sizeof(float));
//  if (!dataset)
//  {
//    fprintf(stderr, "Errore allocazione dataset!\n");
//    return;
//  }
//
//  for (int i = 0; i < N; i++)
//    for (int j = 0; j < D; j++)
//      dataset[i * D + j] = ((float)rand() / RAND_MAX) * 20 - 10;
//
//  // 2. Calcola pivot e pre-quantizzazione
//  float *pivot = calcoloPivot(dataset, h, N, D);
//  if (!pivot)
//  {
//    fprintf(stderr, "Errore calcolo pivot!\n");
//    free(dataset);
//    return;
//  }
//
//  preQuantizeDataset(dataset);
//  preQuantizePivots(pivot);
//
//  // 3. Costruisci indice
//  float *vettoreIndexing = indexing(pivot, dataset);
//  if (!vettoreIndexing)
//  {
//    fprintf(stderr, "Errore indexing!\n");
//    free(dataset);
//    free(pivot);
//    freePreQuantization();
//    return;
//  }
//
//  // 4. Genera query casuale
//  float *query = malloc(D * sizeof(float));
//  if (!query)
//  {
//    fprintf(stderr, "Errore allocazione query!\n");
//    free(dataset);
//    free(pivot);
//    free(vettoreIndexing);
//    freePreQuantization();
//    return;
//  }
//
//  for (int i = 0; i < D; i++)
//    query[i] = ((float)rand() / RAND_MAX) * 20 - 10;
//
//  // 5. Esegui KNN approssimato
//  float *KNN = querying2(query, pivot, dataset, vettoreIndexing);
//  if (!KNN)
//  {
//    fprintf(stderr, "Errore querying!\n");
//    free(dataset);
//    free(pivot);
//    free(vettoreIndexing);
//    free(query);
//    freePreQuantization();
//    return;
//  }
//
//  printf("\nK-NN trovati da querying (id, distanza reale):\n");
//  for (int i = 0; i < k; i++)
//  {
//    int id = (int)KNN[i * 2];
//    float dist = KNN[i * 2 + 1];
//    if (id != -1)
//      printf("%d: %.4f\n", id, dist);
//  }
//
//  // 6. Calcola distanze euclidee reali - USA UN ARRAY SEPARATO
//  float *realDistances = malloc(N * sizeof(float));
//  if (!realDistances)
//  {
//    fprintf(stderr, "Errore allocazione realDistances!\n");
//    free(dataset);
//    free(pivot);
//    free(vettoreIndexing);
//    free(KNN);
//    free(query);
//    freePreQuantization();
//    return;
//  }
//
//  // FIXME: la distanza va calcolata tra le query e i pivot, non query e TUTTO il dataset
//  // CALCOLA TUTTE LE DISTANZE
//  for (int i = 0; i < N; i++)
//  {
//    float *v = &dataset[i * D];
//    realDistances[i] = dEuclidea(query, v);
//  }
//
//  // 7. Crea una COPIA per l'ordinamento
//  float *sortedDistances = malloc(N * sizeof(float));
//  if (!sortedDistances)
//  {
//    fprintf(stderr, "Errore allocazione sortedDistances!\n");
//    free(realDistances);
//    free(dataset);
//    free(pivot);
//    free(vettoreIndexing);
//    free(KNN);
//    free(query);
//    freePreQuantization();
//    return;
//  }
//  memcpy(sortedDistances, realDistances, N * sizeof(float));
//
//  printf("\nK-NN reali (distanza euclidea):\n");
//  for (int n = 0; n < k; n++)
//  {
//    int min_idx = -1;
//    float min_val = FLT_MAX;
//    for (int i = 0; i < N; i++)
//    {
//      if (sortedDistances[i] < min_val)
//      {
//        min_val = sortedDistances[i];
//        min_idx = i;
//      }
//    }
//    if (min_idx != -1)
//    {
//      printf("%d: %.4f\n", min_idx, min_val);
//      sortedDistances[min_idx] = FLT_MAX;
//    }
//  }
//
//  // 8. Verifica errori - USA realDistances ORIGINALE
//  float max_dist_knn = 0.0f;
//  for (int i = 0; i < k; i++)
//    if (KNN[i * 2 + 1] > max_dist_knn)
//      max_dist_knn = KNN[i * 2 + 1];
//
//  int errori = 0;
//  for (int i = 0; i < N; i++)
//  {
//    int in_knn = 0;
//    for (int j = 0; j < k; j++)
//      if ((int)KNN[j * 2] == i)
//      {
//        in_knn = 1;
//        break;
//      }
//
//    if (!in_knn)
//    {
//      // USA realDistances che non è stato modificato!
//      float dist = realDistances[i];
//      if (dist < max_dist_knn)
//      {
//        printf("ERRORE: punto %d escluso ma distanza %.4f < %.4f\n",
//               i, dist, max_dist_knn);
//        errori++;
//      }
//    }
//  }
//
//  if (errori == 0)
//    printf("✓ Nessun errore! Tutti i punti esclusi sono effettivamente più lontani.\n");
//  else
//    printf("✗ Trovati %d errori!\n", errori);
//
//  // 9. Libera memoria
//  free(sortedDistances);
//  free(realDistances);
//  free(dataset);
//  free(pivot);
//  free(vettoreIndexing);
//  free(KNN);
//  free(query);
//  freePreQuantization();
//}

int main(int argc, char **argv)
{
  //srand((unsigned)time(NULL));
  //if (argc < 6) {
  //  N = 2000;
  //  D = 256;
  //  h = 16;
  //  x = 8;
  //  k = 8;
  //} else {
  //  N = atoi(argv[1]);
  //  D = atoi(argv[2]);
  //  h = atoi(argv[3]);
  //  x = atoi(argv[4]);
  //  k = atoi(argv[5]);
  //}
  //if (argc > 6)
  //{
  //  printf("Inserisci numero di valori appropritato");
  //}


  // ================= Parametri di ingresso =================
  char* dsfilename = "ds.ds2";
  char* queryfilename = "query.ds2";
  h = 2;
  k = 3;
  x = 2;
  silent = 0;
  // =========================================================

  params* input = malloc(sizeof(params));

  input->DS = load_data(dsfilename, &input->N, &input->D);
  input->Q = load_data(queryfilename, &input->nq, &input->D);
  input->id_nn = _mm_malloc(input->nq*input->k*sizeof(int), align);
  input->dist_nn = _mm_malloc(input->nq*input->k*sizeof(type), align);
  input->h = h;
  input->k = k;
  input->x = x;
  input->silent = silent;

  N = input->N;
  D = input->D;
  printf("N = %d, D = %d, h = %d, x = %d, k = %d\n", N,D,h,x,k);

  clock_t t;
  float time;

  t = omp_get_wtime();
  // =========================================================
  fit(input);
  // =========================================================
  t = omp_get_wtime() - t;
  time = ((float)t)/CLOCKS_PER_SEC;

  if(!input->silent)
    printf("FIT time = %.5f secs\n", time);
  else
    printf("%.3f\n", time);

  t = omp_get_wtime();
  // =========================================================
  predict(input);
  // =========================================================
  t = omp_get_wtime() - t;
  time = ((float)t)/CLOCKS_PER_SEC;

  if(!input->silent)
    printf("PREDICT time = %.5f secs\n", time);
  else
    printf("%.3f\n", time);

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
  float *realDistances = malloc(input->nq*k*sizeof(float));
  for (int i = 0; i < input->nq; i++)
  {
    float *v = &input->DS[i * D];
    for(int j = 0; j < k; j++) {
      int idx = input->id_nn[i*k+j];
      float *genericoVettore = &input->DS[idx];
      realDistances[i*k+j] = dEuclidea(v,genericoVettore, D);
    }
  }
  printf("\n--- CONFRONTO DISTANZE (nq: %d, k: %d) ---\n", input->nq, k);

  // FIX: Allocazione temporanea per il confronto (opzionale, calcoliamo al volo)
  for (int i = 0; i < input->nq; i++) {
    printf("Query #%d:\n", i);

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

      printf("  NN %d (ID %d): Real: %10.5f | Stored: %10.5f | Diff: %e\n", 
          j, idx_neighbor, real, stored, real - stored);
    }
    printf("\n"); 
  }

  // Pulizia finale (tutto corretto ora che calcoloPivot usa _mm_malloc)
  _mm_free(input->DS);
  _mm_free(input->Q);
  _mm_free(input->P);
  _mm_free(input->index);
  _mm_free(input->id_nn);
  _mm_free(input->dist_nn);
  free(input);



  // nomeFittizio();
  // testDistanzaApprossimata();
  // testIndexing();
  //testQueryingCompleto();
  return 0;
}
