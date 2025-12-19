#include <stdlib.h>
#include <string.h>
#include <xmmintrin.h>
#include <omp.h>
#include "common.h"
#include <math.h>
#include <stdio.h>
#include <stdint.h>

const int INDEXING_PROCEDURE_ERROR = -1;
const int BLOCK_SIZE = 512;

// Variabili globali per i dati quantizzati
uint32_t* vPlus_all = NULL;
uint32_t* vMinus_all = NULL;
uint32_t* pPlus = NULL;
uint32_t* pMinus = NULL;

int num_blocchi_global = 0;

/* Ma che vuol dire che in C non c'è l'overloading della funzioni... -> https://en.cppreference.com/w/c/language/generic.html*/
extern int andBitABit(int v, int w);
extern float prodScalaref(float *v, float *w, int D);
extern double prodScalared(double *v, double *w, int D);
#define prodScalare(v,w,D) _Generic((v), float*: prodScalaref, double*:prodScalared)(v,w,D) 
extern float dEuclideaf(float *v, float *w, int D);
extern double dEuclidead(double *v, double *w, int D);
#define dEuclidea(v,w,D) _Generic((v), float*: dEuclideaf, double*:dEuclidead)(v,w,D) 

/*GENERALIZZO IL PROGRAMMA PER FUNZIONARE SIA CON DOUBLE CHE CON FLOAT*/
#define ABS(x) _Generic((x), float: fabsf, double: fabs)(x)

int ottieniNumBitUno(uint32_t n) {
    int count = 0;
    while (n > 0) {
        if (n & 1) { 
            count++;
        }
        n >>= 1; //esempio: 1101 >> 1 = 0110
    }
    return count;
}

// Funzione di pulizia (chiamata alla fine di predict)
void freePreQuantization()
{
  if (vPlus_all) { free(vPlus_all); vPlus_all = NULL; }
  if (vMinus_all) { free(vMinus_all); vMinus_all = NULL; }
  if (pPlus) { free(pPlus); pPlus = NULL; }
  if (pMinus) { free(pMinus); pMinus = NULL; }
}

// Gestione lista ordinata K-NN
void insert_into_knn(VECTOR KNN, int k, int id, type distance)
{
  type max_distance = -1.0f;
  int max_index_id = -1;

  // Trova il vicino più lontano attualmente in lista (il candidato ad uscire)
  for (int i = 0; i < k; i++)
  {
    int index_dist = (i * 2) + 1;
    type current_distance = KNN[index_dist];

    if (current_distance > max_distance)
    {
      max_distance = current_distance;
      max_index_id = i * 2;
    }
  }

  // Se la nuova distanza è minore del peggiore attuale, sostituisci
  if (distance < max_distance)
  {
    KNN[max_index_id] = (type)id;
    KNN[max_index_id + 1] = distance;
  }
}

// Recupera la distanza massima attuale nella lista K-NN (il raggio di ricerca)
type get_d_k_max(VECTOR KNN, int k)
{
  type max_distance = -1.0f;
  for (int i = 0; i < k; i++)
  {
    type current_distance = KNN[(i * 2) + 1];
    if (current_distance > max_distance)
    {
      max_distance = current_distance;
    }
  }
  return max_distance;
}

// Calcolo distanza approssimata (Eq. 2 del documento)
type distanzaApprossimataPreQ(uint32_t* vPlus, uint32_t* vMinus, uint32_t* wPlus, uint32_t* wMinus, int D)
{
    int totale_bit_1 = 0;

    // Iteriamo su ogni blocco di interi
    for(int b = 0; b < num_blocchi_global; b++) {
        
        // 1. Chiamata Assembly per il blocco corrente
        // Passiamo i singoli interi del bucket 'b'
        int posPosVal = andBitABit(vPlus[b], wPlus[b]);
        int negNegVal = andBitABit(vMinus[b], wMinus[b]);
        int posNegVal = andBitABit(vPlus[b], wMinus[b]);
        int negPosVal = andBitABit(vMinus[b], wPlus[b]);

        // 2. Conteggio bit e accumulo nel totale
        // Sommiamo (posPos + negNeg - posNeg - negPos) per questo blocco
        totale_bit_1 += __builtin_popcount(posPosVal);
        totale_bit_1 += __builtin_popcount(negNegVal);
        totale_bit_1 -= __builtin_popcount(posNegVal);
        totale_bit_1 -= __builtin_popcount(negPosVal);
    }

    return (type)totale_bit_1;
}

// Costruzione indice (distanze dataset <-> pivot)
VECTOR indexing(params* input)
{
  int N = input->N;
  int h = input->h;
  int D = input->D;

  VECTOR output = _mm_malloc(N * h * sizeof(type), 32); 

  if (output == NULL) return NULL;

  for (int r = 0; r < N; r++)
  {
    uint32_t* vPlus = &vPlus_all[r * num_blocchi_global]; 
    uint32_t* vMinus = &vMinus_all[r * num_blocchi_global];
    for (int c = 0; c < h; c++)
    {
      uint32_t* pPlusC = &pPlus[c * num_blocchi_global];
      uint32_t* pMinusC = &pMinus[c * num_blocchi_global];
      output[r * h + c] = distanzaApprossimataPreQ(vPlus, vMinus, pPlusC, pMinusC, D);
    }
  }
  return output;
}

// Funzione di quantizzazione (versione HEAD)
void quantizing(VECTOR v, uint32_t *vMinus, uint32_t *vPlus, params* input, int *array_indici)
{
  int D = input->D;
  int x = input->x;

  // 1. Reset
  for(int b = 0; b < num_blocchi_global; b++) {
      vPlus[b] = 0;
      vMinus[b] = 0;
  }

  for (int k = 0; k < D; k++)
  {
    array_indici[k] = k;
  }

  // 2. Cerco gli X elementi con valore assoluto massimo (Partial Selection Sort)
  for (int i = 0; i < x; i++)
  {
    int maxIndex = i;
    type maxVal = ABS(v[array_indici[i]]); 

    for (int j = i + 1; j < D; j++)
    {
      type currentVal = ABS(v[array_indici[j]]);

      if (currentVal > maxVal)
      {
        maxVal = currentVal;
        maxIndex = j;
      }
    }

    // Scambio solo gli indici
    int temp = array_indici[i];
    array_indici[i] = array_indici[maxIndex];
    array_indici[maxIndex] = temp;
  }

  // 3. Assegnazione ai vettori vPlus e vMinus
  for (int i = 0; i < x; i++)
  {
    int original_idx = array_indici[i]; 
    
    int bucket = original_idx / 32;
    int esponente_locale = original_idx % 32;

    uint32_t valore_posizionale = (uint32_t)pow(2, esponente_locale);

    if (v[original_idx] >= 0) {
      vPlus[bucket] += valore_posizionale; 
    } else {
      vMinus[bucket] += valore_posizionale;
    }
  }

}

// Selezione Pivot
// FIXME: qui ho un dubbio, devo mettere VECTOR al dataset o MATRIX?
int *calcoloPivot(VECTOR dataSet, int h, int N, int D)
{
  printf("INIZIO CALCOLO PIVOT\n");
  int *pivot = (int *)_mm_malloc(h * sizeof(int), 32); 

  if (!pivot) return NULL; 

  /* README: il motivo per cui faccio N/h senza applicare l'istruzione di floorf
     è perché già in automatico la divisione tra due float porta a troncamento per difetto,
     c'è un solo caso in cui non porta a questo risultato, se i numeri sono negativi, siccome
     qui sono sempre positivi (N e h) allora non ci preoccupiamo di quel caso limite
     es.: 10/6 -> 1.666 -> 1, ed è vero sia se facciamo N/h che floorf((float)N/h)
     invece se facessimo -35/10 -> -3.5 -> se lasciamo N/h diventa = -3, con floorf -> -4
     int offset = (int)floorf((float)N / h); */
  int offset = N/h;
  for (int i = 0; i < h; i++)
    pivot[i] = i * offset;

  printf("FINE CALCOLO PIVOT\n");
  return pivot;
}

// Pre-quantizzazione intero Dataset
void preQuantizeDataset(params *input)
{
  int N = input->N;
  int D = input->D;
  vPlus_all = calloc((size_t)N * num_blocchi_global, sizeof(uint32_t));
  vMinus_all = calloc((size_t)N * num_blocchi_global, sizeof(uint32_t));

  if (!vPlus_all || !vMinus_all) {
    fprintf(stderr, "Errore allocazione vPlus_all/vMinus_all\n");
    if (vPlus_all) free(vPlus_all);
    if (vMinus_all) free(vMinus_all);
    vPlus_all = vMinus_all = NULL;
    return;
  }

  int *idx_buff = malloc(D * sizeof(int));

  for (int i = 0; i < N; i++)
  {
    VECTOR v = &input->DS[i * D]; // Vettore corrente del dataset
    uint32_t* vp = &vPlus_all[i * num_blocchi_global]; // Vettore vPlus da quantizzare, preso dalla matrice globale
    uint32_t* vm = &vMinus_all[i * num_blocchi_global]; // Vettore vMinus da quantizzare, preso dalla matrice globale
    quantizing(v, vm, vp, input, idx_buff);
  }

  free(idx_buff);
}

// Pre-quantizzazione dei Pivot
void preQuantizePivots(params *input)
{
  int D = input->D;
  int h = input->h;

  if (!pPlus || !pMinus) {
    fprintf(stderr, "Errore allocazione pPlus/pMinus\n");
    if (pPlus) free(pPlus);
    if (pMinus) free(pMinus);
    pPlus = pMinus = NULL;
    return;
  }

  int *idx_buff = malloc(D * sizeof(int));

  for (int i = 0; i < h; i++)
  {
    int pivot_idx = input->P[i];
    
    uint32_t* pP = &pPlus[i * num_blocchi_global];
    uint32_t* pM = &pMinus[i * num_blocchi_global];

    if (pivot_idx < 0 || pivot_idx >= input->N) {
      // Gestione errore pivot fuori range
      for(int b=0; b < num_blocchi_global; b++) { pP[b]=0; pM[b]=0; }
      continue;
    }
    VECTOR p = &input->DS[pivot_idx * D];
    
    quantizing(p, pM, pP, input, idx_buff);
  }
  free(idx_buff);
}

// Processa un blocco di dataset [start_N, end_N) per una specifica query
void process_block_for_query(int start_N, int end_N, VECTOR query, params *input, uint32_t* qPlus, uint32_t* qMinus, VECTOR dQP, VECTOR KNN) 
{
  int D = input->D;
  int h = input->h;
  int k = input->k;

  // Itera SOLO sul blocco corrente del dataset
  for (int i = start_N; i < end_N; i++)
  {
    // A. Calcolo Lower Bound coi Pivot 
    type best_lb = 0.0;
    int j;

    VECTOR current_index_row = &input->index[i * h];

    type local_best = best_lb;
    for (j = 0; j <= h-4; j+=4)
    {
      type val0 = ABS(current_index_row[j]   - dQP[j]);
      type val1 = ABS(current_index_row[j+1] - dQP[j+1]);
      type val2 = ABS(current_index_row[j+2] - dQP[j+2]);
      type val3 = ABS(current_index_row[j+3] - dQP[j+3]);

      // Controlli separati (la CPU moderna gestisce bene questi branch se rari)
      if (val0 > local_best) local_best = val0;
      if (val1 > local_best) local_best = val1;
      if (val2 > local_best) local_best = val2;
      if (val3 > local_best) local_best = val3;
    }
    for (; j < h; j++) {
      type val = ABS(current_index_row[j] - dQP[j]);
      if (val > local_best) local_best = val;
    }

    best_lb = local_best;

    type d_k_max = 0.0;
    type max_distance = -1.0;
    for (int i = 0; i < k; i++)
    {
      type current_distance = KNN[(i * 2) + 1];
      if (current_distance > max_distance)
      {
        max_distance = current_distance;
      }
    }
    d_k_max = max_distance;

    if (best_lb >= d_k_max)
      continue;

    uint32_t* vPlus = &vPlus_all[i * num_blocchi_global];
    uint32_t* vMinus = &vMinus_all[i * num_blocchi_global];

    type d_q_v_approx = distanzaApprossimataPreQ(vPlus, vMinus, qPlus, qMinus, D);

    // D. Inserimento
    if (d_q_v_approx < d_k_max)
    {
      insert_into_knn(KNN, k, i, d_q_v_approx);
    }
  }
}


void fit(params* input){
  
  num_blocchi_global = (input->D + 31) / 32;

  // Selezione dei pivot
  printf("INIZIO SELEZIONE PIVOT\n");
  input->P = calcoloPivot(input->DS, input->h, input->N, input->D);
  if (!input->P)
  {
    fprintf(stderr, "Errore calcolo pivot!\n");
    if (input->DS) _mm_free(input->DS);
    return;
  }
  printf("FINE SELEZIONE PIVOT\n");

  // pre-quantizzazione dataset
  printf("INIZIO PRE-QUANTIZZAZIONE DATASET\n");
  preQuantizeDataset(input);
  if (!vPlus_all || !vMinus_all) {
    fprintf(stderr, "Errore preQuantizeDataset: allocazione fallita\n");
    if (input->P) _mm_free(input->P);
    if (vPlus_all) free(vPlus_all);
    if (vMinus_all) free(vMinus_all);
    return;
  }
  printf("FINE PRE-QUANTIZZAZIONE DATASET\n");

  pPlus = calloc(input->h * num_blocchi_global, sizeof(uint32_t));
  pMinus = calloc(input->h * num_blocchi_global, sizeof(uint32_t));

  // pre-quantizzazione pivot
  printf("INIZIO PRE-QUANTIZZAZIONE PIVOT\n");
  preQuantizePivots(input);
  if (!pPlus || !pMinus) {
    fprintf(stderr, "Errore preQuantizePivots: allocazione fallita\n");
    if (input->P) _mm_free(input->P);
    freePreQuantization();
    return;
  }
  printf("FINE PRE-QUANTIZZAZIONE PIVOT\n");

  // Costruzione dell'indice
  printf("INIZIO COSTRUZIONE INDICE\n");
  input->index = indexing(input); 
  if (!input->index)
  {
    fprintf(stderr, "Errore indexing!\n");
    if (input->P) _mm_free(input->P);
    freePreQuantization();
    exit(INDEXING_PROCEDURE_ERROR);
  }
  printf("FINE COSTRUZIONE INDICE\n");
}

void predict(params* input){
  int nq = input->nq;
  int N = input->N;
  int D = input->D;
  int k = input->k;
  int h = input->h;

  // Allocazione buffer temporanei FUORI dal ciclo per performance
  // Questo abilita la logica usata in querying2
  uint32_t* qPlusAll = malloc(nq * num_blocchi_global * sizeof(uint32_t));
  uint32_t* qMinusAll = malloc(nq * num_blocchi_global * sizeof(uint32_t));
  VECTOR dQPAll = malloc(h * nq * sizeof(type));

  VECTOR KNNAll = malloc(2*k * nq * sizeof(type));

  int *idx_buff = malloc(D * sizeof(int)); 
  for(int q = 0; q < nq; q++) {
    VECTOR query = &input->Q[q * D];
    uint32_t* qPlus = &qPlusAll[q * num_blocchi_global];
    uint32_t* qMinus = &qMinusAll[q * num_blocchi_global];
    VECTOR dQP = &dQPAll[q * h];
    VECTOR currentKNN = &KNNAll[q * 2 * k];

    // Init KNN a infinito
    for (int i = 0; i < k; i++)
    {
      currentKNN[2 * i] = -1.0f;     // ID
      currentKNN[2 * i + 1] = INFINITY; // Distanza
    }

    // Quantizza query
    quantizing(query, qMinus, qPlus, input, idx_buff);

    // Precalcola distanze query-pivot
    for (int j = 0; j < h; j++) {
      uint32_t* pPlusC = &pPlus[j * num_blocchi_global]; 
      uint32_t* pMinusC = &pMinus[j * num_blocchi_global];
      dQP[j] = distanzaApprossimataPreQ(qPlus, qMinus, pPlusC, pMinusC, D);
    }
  } 
  free(idx_buff);

  // iteriamo i blocchi di dataset in ram e calcoliamo tutte le query
  for (int idxStart = 0; idxStart < N; idxStart += BLOCK_SIZE) {
    int idxEnd = idxStart + BLOCK_SIZE;
    if(idxEnd > N) idxEnd = N;
    for(int q = 0; q < nq; q++) {
      VECTOR query = &input->Q[q * D]; 
      uint32_t* qPlus = &qPlusAll[q * num_blocchi_global];
      uint32_t* qMinus = &qMinusAll[q * num_blocchi_global];
      VECTOR dQP = &dQPAll[q * h];
      VECTOR current_KNN = &KNNAll[q * 2 * k];

      // Aggiorna lo stato KNN della query q con i dati del blocco dataset corrente
      process_block_for_query(idxStart, idxEnd, query, input, qPlus, qMinus, dQP, current_KNN);
    }
  }

  for(int q = 0; q < nq; q++) {
    VECTOR query = &input->Q[q * D];
    VECTOR current_KNN = &KNNAll[q * 2 * k];

    // Calcola distanze euclidee reali per i k candidati rimasti
    for (int i = 0; i < k; i++)
    {
      int id_vicino = (int)current_KNN[2 * i];

      if (id_vicino >= 0) {
        VECTOR v = &input->DS[id_vicino * D];
        current_KNN[2 * i + 1] = dEuclidea(query, v, D); 
      }
    }
    // Scrittura output finale
    for (int j = 0; j < k; j++) {
      input->id_nn[q*k + j] = (int) current_KNN[2*j];      
      input->dist_nn[q*k + j] = current_KNN[2*j + 1];     
    }
  }

  // Pulizia
  free(KNNAll);
  free(qPlusAll);
  free(qMinusAll);
  free(dQPAll);
  freePreQuantization();

}
