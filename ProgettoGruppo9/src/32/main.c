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
  char* dsfilename = "generated_dataset.ds2";
  char* queryfilename = "generated_queries.ds2";
  h = 21;
  k = 7;
  x = 23;
  silent = 1;
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
	double time;

	t = omp_get_wtime();
	// =========================================================
	fit(input);
	// =========================================================
	t = omp_get_wtime() - t;
	time = ((float)t)/CLOCKS_PER_SEC;

	if(!input->silent)
		printf("FIT time = %.5lf secs\n", time);
	else
		printf("%.3lf\n", time);

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




printf("\n=== VALUTAZIONE K-NN APPROSSIMATI ===\n");

// Prendi la prima query
float *queryVec = &input->Q[0 * D];

// --- STEP 1: Calcola TUTTI i k-NN REALI (brute force) ---
float *allDistances = malloc(input->N * sizeof(float));
int *allIndices = malloc(input->N * sizeof(int));

for(int i = 0; i < input->N; i++) {
    float *vec = &input->DS[i * D];
    allDistances[i] = dEuclidea(queryVec, vec, D);
    allIndices[i] = i;
}

// Ordina per trovare i veri k-NN (selection sort sui primi k)
for(int i = 0; i < k; i++) {
    int min_idx = i;
    for(int j = i+1; j < input->N; j++) {
        if(allDistances[j] < allDistances[min_idx]) {
            min_idx = j;
        }
    }
    // Swap distanze
    float tmpD = allDistances[i];
    allDistances[i] = allDistances[min_idx];
    allDistances[min_idx] = tmpD;
    // Swap indici
    int tmpI = allIndices[i];
    allIndices[i] = allIndices[min_idx];
    allIndices[min_idx] = tmpI;
}

// --- STEP 2: Confronta con i k-NN trovati dal tuo algoritmo ---
printf("\nK-NN REALI (brute force):\n");
for(int i = 0; i < k; i++) {
    printf("  %d) ID=%4d  dist=%.6f\n", i, allIndices[i], allDistances[i]);
}

printf("\nK-NN APPROSSIMATI (tuo algoritmo):\n");
for(int i = 0; i < k; i++) {
    int id = input->id_nn[0*k + i];
    float dist = input->dist_nn[0*k + i];
    printf("  %d) ID=%4d  dist=%.6f\n", i, id, dist);
}

// --- STEP 3: Calcola metriche ---
int corretti = 0;
for(int i = 0; i < k; i++) {
    int id_trovato = input->id_nn[0*k + i];
    // Verifica se è nei veri k-NN
    for(int j = 0; j < k; j++) {
        if(allIndices[j] == id_trovato) {
            corretti++;
            break;
        }
    }
}

float recall = (float)corretti / k;
printf("\n--- METRICHE ---\n");
printf("Recall@%d: %.2f%% (%d/%d corretti)\n", k, recall*100, corretti, k);

// Confronta le distanze massime
float max_dist_reale = allDistances[k-1];
float max_dist_approx = input->dist_nn[0*k + k-1];
printf("Distanza massima (reale):  %.6f\n", max_dist_reale);
printf("Distanza massima (approx): %.6f\n", max_dist_approx);
printf("Errore relativo: %.2f%%\n", 
       100.0 * fabs(max_dist_approx - max_dist_reale) / max_dist_reale);

free(allDistances);
free(allIndices);

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





  printf("\n=== TEST SU TUTTE LE QUERY ===\n");
int totale_corretti = 0;
float errore_totale = 0.0;

for(int q = 0; q < input->nq; q++) {
    float *queryVec = &input->Q[q * D];
    
    // Calcola k-NN reali
    float *allDistances = malloc(input->N * sizeof(float));
    int *allIndices = malloc(input->N * sizeof(int));
    
    for(int i = 0; i < input->N; i++) {
        float *vec = &input->DS[i * D];
        allDistances[i] = dEuclidea(queryVec, vec, D);
        allIndices[i] = i;
    }
    
    // Ordina
    for(int i = 0; i < k; i++) {
        int min_idx = i;
        for(int j = i+1; j < input->N; j++) {
            if(allDistances[j] < allDistances[min_idx]) {
                min_idx = j;
            }
        }
        float tmpD = allDistances[i];
        allDistances[i] = allDistances[min_idx];
        allDistances[min_idx] = tmpD;
        int tmpI = allIndices[i];
        allIndices[i] = allIndices[min_idx];
        allIndices[min_idx] = tmpI;
    }
    
    // Conta corretti
    int corretti = 0;
    for(int i = 0; i < k; i++) {
        int id_trovato = input->id_nn[q*k + i];
        for(int j = 0; j < k; j++) {
            if(allIndices[j] == id_trovato) {
                corretti++;
                break;
            }
        }
    }
    totale_corretti += corretti;
    
    float max_dist_reale = allDistances[k-1];
    float max_dist_approx = input->dist_nn[q*k + k-1];
    errore_totale += fabs(max_dist_approx - max_dist_reale) / max_dist_reale;
    
    free(allDistances);
    free(allIndices);
}

float recall_medio = (float)totale_corretti / (input->nq * k);
float errore_medio = errore_totale / input->nq;

printf("Recall medio: %.2f%%\n", recall_medio * 100);
printf("Errore relativo medio: %.2f%%\n", errore_medio * 100);
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
