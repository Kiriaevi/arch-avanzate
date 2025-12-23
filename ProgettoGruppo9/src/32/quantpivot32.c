#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <xmmintrin.h>
#include <omp.h>
#include "common.h"
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>


const int INDEXING_PROCEDURE_ERROR = -1;
const int BLOCK_SIZE = 512;
const int BATCH_QUERY = 32;

// Variabili globali per i dati quantizzati
uint32_t *vPlus_all = NULL;
uint32_t *vMinus_all = NULL;
uint32_t *pPlus = NULL;
uint32_t *pMinus = NULL;

int num_blocchi_global = 0;

/* Ma che vuol dire che in C non c'è l'overloading della funzioni... -> https://en.cppreference.com/w/c/language/generic.html*/
extern double trovaMassimod(double *current_index_row, double *dQP, int h);
extern float trovaMassimof(float *current_index_row, float *dQP, int h);
#define trovaMassimo(curr_index_row, dQP, h) _Generic((curr_index_row), float *: trovaMassimof, double *: trovaMassimod)(curr_index_row, dQP, h)
extern int distApprossimata(uint32_t *vPlus, uint32_t *vMinus, uint32_t *wPlus, uint32_t *wMinus, int D);
extern float dEuclideaf(float *v, float *w, int D);
extern double dEuclidead(double *v, double *w, int D);
#define dEuclidea(v, w, D) _Generic((v), float *: dEuclideaf, double *: dEuclidead)(v, w, D)
extern float get_d_k_maxf(float *KNN, int k);
extern double get_d_k_maxd(double *KNN, int k);
#define get_d_k_max(KNN, k) _Generic((KNN), float *: get_d_k_maxf, double *: get_d_k_maxd)(KNN, k)


/*GENERALIZZO IL PROGRAMMA PER FUNZIONARE SIA CON DOUBLE CHE CON FLOAT*/
#define ABS(x) _Generic((x), float: fabsf, double: fabs)(x)

// Funzione di pulizia (chiamata alla fine di predict)
void freePreQuantization()
{
  if (vPlus_all)
  {
    free(vPlus_all);
    vPlus_all = NULL;
  }
  if (vMinus_all)
  {
    free(vMinus_all);
    vMinus_all = NULL;
  }
  if (pPlus)
  {
    free(pPlus);
    pPlus = NULL;
  }
  if (pMinus)
  {
    free(pMinus);
    pMinus = NULL;
  }
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
    KNN[max_index_id] = id;
    KNN[max_index_id + 1] = distance;
  }
}

// Recupera la distanza massima attuale nella lista K-NN (il raggio di ricerca)
// Costruzione indice (distanze dataset <-> pivot)
VECTOR indexing(params *input)
{
  int N = input->N;
  int h = input->h;
  int D = input->D;

  // TODO:
  // perché allineare a 32 byte?
  VECTOR output = _mm_malloc(N * h * sizeof(type), 32); 

  if (output == NULL)
    return NULL;

  for (int r = 0; r < N; r++)
  {
    uint32_t *vPlus = &vPlus_all[r * num_blocchi_global];
    uint32_t *vMinus = &vMinus_all[r * num_blocchi_global];
    for (int c = 0; c < h; c++)
    {
      uint32_t* pPlusC = &pPlus[c * num_blocchi_global];
      uint32_t* pMinusC = &pMinus[c * num_blocchi_global];
      output[r * h + c] = distApprossimata(vPlus, vMinus, pPlusC, pMinusC, num_blocchi_global);
    }
  }
  return output;
}

/*
   Stabilisco chi tra i due figli è il minimo.
   Se effettivamente esiste qualcuno di più 
   piccolo allora faccio uno swap, altrimenti
   fermo il ciclo.
   Gli elementi "in cima" sono i più piccoli.
*/
static inline void heapify(VECTOR v, int *indices, int n, int i) {
    while (true) {
        int smallest = i;
        int left = 2 * i + 1;
        int right = 2 * i + 2;

        if (left < n && ABS(v[indices[left]]) < ABS(v[indices[smallest]]))             
          smallest = left;
        if (right < n && ABS(v[indices[right]]) < ABS(v[indices[smallest]]))             
          smallest = right;
        if (smallest != i) {
            int temp = indices[i];
            indices[i] = indices[smallest];
            indices[smallest] = temp;

            i = smallest;
        } else {
            break;
        }
    }
}

void quantizing(VECTOR v, uint32_t *vPlus, uint32_t *vMinus, params *input, int *array_indici)
{
    int D = input->D;
    int x = input->x;
    for (int b = 0; b < num_blocchi_global; b++) {
        vPlus[b] = 0;
        vMinus[b] = 0;
    }

    for (int k = 0; k < D; k++) {
        array_indici[k] = k;
    }

    // uso HEAP SELECTION se x non è troppo grande rispetto a D
    int initHeapSize = ((x-1)-1) / 2;
    for (int i = initHeapSize; i >= 0; i--) {
      heapify(v, array_indici, x, i);
    }
    for (int i = x; i < D; i++)             
      /* Siccome alla fine siamo interessati ai valori massimi in valore assoluto,
       * quando troviamo qualcuno di più grande lo scambiamo con la radice dell'heap
       * e se necessario lo spostiamo in fondo all'heap (solo i più piccoli valori stanno in cima)
       */
      if (ABS(v[array_indici[i]]) > ABS(v[array_indici[0]])) {
        int temp = array_indici[0];
        array_indici[0] = array_indici[i];
        array_indici[i] = temp;
        heapify(v, array_indici, x, 0);
      }
    /*
        // per ora lascio il partial sort
    for (int i = 0; i < x; i++) {
      int maxIndex = i;
      double maxVal = ABS(v[array_indici[i]]); 
      for (int j = i + 1; j < D; j++) {
        double currentVal = ABS(v[array_indici[j]]);
        if (currentVal > maxVal) {
          maxVal = currentVal;
          maxIndex = j;
        }
      }
      // Scambio solo gli indici
      int temp = array_indici[i];
      array_indici[i] = array_indici[maxIndex];
      array_indici[maxIndex] = temp;
    }
    */

    // Assegnazione ai vettori vPlus e vMinus
    for (int i = 0; i < x; i++)
    {
        int original_idx = array_indici[i];

        // Divisione intera e modulo per individuare il blocco e il bit
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

int *calcoloPivot(VECTOR dataSet, int h, int N, int D)
{
  printf("INIZIO CALCOLO PIVOT\n");
  int *pivot = (int *)_mm_malloc(h * sizeof(int), 32);

  if (!pivot)
    return NULL;

  /* README: il motivo per cui faccio N/h senza applicare l'istruzione di floorf
     è perché già in automatico la divisione tra due float porta a troncamento per difetto,
     c'è un solo caso in cui non porta a questo risultato, se i numeri sono negativi, siccome
     qui sono sempre positivi (N e h) allora non ci preoccupiamo di quel caso limite
     es.: 10/6 -> 1.666 -> 1, ed è vero sia se facciamo N/h che floorf((float)N/h)
     invece se facessimo -35/10 -> -3.5 -> se lasciamo N/h diventa = -3, con floorf -> -4
     int offset = (int)floorf((float)N / h); */
  int offset = N / h;
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

  if (!vPlus_all || !vMinus_all)
  {
    fprintf(stderr, "Errore allocazione vPlus_all/vMinus_all\n");
    if (vPlus_all)
      free(vPlus_all);
    if (vMinus_all)
      free(vMinus_all);
    vPlus_all = vMinus_all = NULL;
    return;
  }

  int *idx_buff = malloc(D * sizeof(int));

  for (int i = 0; i < N; i++)
  {
    VECTOR v = &input->DS[i * D];                       // Vettore corrente del dataset
    uint32_t *vp = &vPlus_all[i * num_blocchi_global];  // Vettore vPlus da quantizzare, preso dalla matrice globale
    uint32_t *vm = &vMinus_all[i * num_blocchi_global]; // Vettore vMinus da quantizzare, preso dalla matrice globale
    quantizing(v, vm, vp, input, idx_buff);
  }

  free(idx_buff);
}

// Pre-quantizzazione dei Pivot
void preQuantizePivots(params *input)
{
  int D = input->D;
  int h = input->h;

  if (!pPlus || !pMinus)
  {
    fprintf(stderr, "Errore allocazione pPlus/pMinus\n");
    if (pPlus)
      free(pPlus);
    if (pMinus)
      free(pMinus);
    pPlus = pMinus = NULL;
    return;
  }

  int *idx_buff = malloc(D * sizeof(int));

  for (int i = 0; i < h; i++)
  {
    int pivot_idx = input->P[i];

    uint32_t *pP = &pPlus[i * num_blocchi_global];
    uint32_t *pM = &pMinus[i * num_blocchi_global];

    if (pivot_idx < 0 || pivot_idx >= input->N)
    {
      // Gestione errore pivot fuori range
      for (int b = 0; b < num_blocchi_global; b++)
      {
        pP[b] = 0;
        pM[b] = 0;
      }
      continue;
    }
    VECTOR p = &input->DS[pivot_idx * D];

    quantizing(p, pM, pP, input, idx_buff);
  }
  free(idx_buff);
}

// Processa un blocco di dataset [start_N, end_N) per una specifica query
void process_block_for_query(int start_N, int end_N, VECTOR query, params *input, 
                             uint32_t* qPlus, uint32_t* qMinus, VECTOR dQP, VECTOR KNN) 
{
    int D = input->D;
    int h = input->h;
    int k = input->k;

    type d_k_max = get_d_k_max(KNN, k);

    // Itera SOLO sul blocco corrente del dataset
    for (int i = start_N; i < end_N; i++)
    {
        type *current_index_row = &input->index[i * h];
        type best_lb = 0.0;
        int j = 0;

        best_lb = trovaMassimo(current_index_row, dQP, h);

        if (best_lb >= d_k_max)
            continue;

        uint32_t* vPlus = &vPlus_all[i * num_blocchi_global];
        uint32_t* vMinus = &vMinus_all[i * num_blocchi_global];

        type d_q_v_approx = distApprossimata(vPlus, vMinus, qPlus, qMinus, num_blocchi_global);

        if (d_q_v_approx < d_k_max)
        {
            insert_into_knn(KNN, k, i, d_q_v_approx);
            d_k_max = get_d_k_max(KNN, k);
        }
    }
}

void fit(params *input)
{

  num_blocchi_global = (input->D + 31) / 32;

  // Selezione dei pivot
  printf("INIZIO SELEZIONE PIVOT\n");
  input->P = calcoloPivot(input->DS, input->h, input->N, input->D);
  if (!input->P)
  {
    fprintf(stderr, "Errore calcolo pivot!\n");
    if (input->DS)
      _mm_free(input->DS);
    return;
  }
  printf("FINE SELEZIONE PIVOT\n");

  // pre-quantizzazione dataset
  printf("INIZIO PRE-QUANTIZZAZIONE DATASET\n");
  preQuantizeDataset(input);
  if (!vPlus_all || !vMinus_all)
  {
    fprintf(stderr, "Errore preQuantizeDataset: allocazione fallita\n");
    if (input->P)
      _mm_free(input->P);
    if (vPlus_all)
      free(vPlus_all);
    if (vMinus_all)
      free(vMinus_all);
    return;
  }
  printf("FINE PRE-QUANTIZZAZIONE DATASET\n");

  pPlus = calloc(input->h * num_blocchi_global, sizeof(uint32_t));
  pMinus = calloc(input->h * num_blocchi_global, sizeof(uint32_t));

  // pre-quantizzazione pivot
  printf("INIZIO PRE-QUANTIZZAZIONE PIVOT\n");
  preQuantizePivots(input);
  if (!pPlus || !pMinus)
  {
    fprintf(stderr, "Errore preQuantizePivots: allocazione fallita\n");
    if (input->P)
      _mm_free(input->P);
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
    if (input->P)
      _mm_free(input->P);
    freePreQuantization();
    exit(INDEXING_PROCEDURE_ERROR);
  }
  printf("FINE COSTRUZIONE INDICE\n");
}

void predict(params *input)
{
  int nq = input->nq;
  int N = input->N;
  int D = input->D;
  int k = input->k;
  int h = input->h;

  // Allocazione buffer temporanei FUORI dal ciclo per performance
  // Questo abilita la logica usata in querying2
  uint32_t *qPlusAll = malloc(nq * num_blocchi_global * sizeof(uint32_t));
  uint32_t *qMinusAll = malloc(nq * num_blocchi_global * sizeof(uint32_t));
  VECTOR dQPAll = malloc(h * nq * sizeof(type));

  VECTOR KNNAll = malloc(2 * k * nq * sizeof(type));

  int *idx_buff = malloc(D * sizeof(int));
  for (int q = 0; q < nq; q++)
  {
    VECTOR query = &input->Q[q * D];
    uint32_t *qPlus = &qPlusAll[q * num_blocchi_global];
    uint32_t *qMinus = &qMinusAll[q * num_blocchi_global];
    VECTOR dQP = &dQPAll[q * h];
    VECTOR currentKNN = &KNNAll[q * 2 * k];

    // Init KNN a infinito
    for (int i = 0; i < k; i++)
    {
      currentKNN[2 * i] = -1.0f;        // ID
      currentKNN[2 * i + 1] = INFINITY; // Distanza
    }

    // Quantizza query
    quantizing(query, qMinus, qPlus, input, idx_buff);

    // Precalcola distanze query-pivot
    for (int j = 0; j < h; j++) {
      uint32_t* pPlusC = &pPlus[j * num_blocchi_global]; 
      uint32_t* pMinusC = &pMinus[j * num_blocchi_global];
      dQP[j] = (type)distApprossimata(qPlus, qMinus, pPlusC, pMinusC, num_blocchi_global);
    }
  }
  free(idx_buff);


#pragma omp parallel for
  for (int qStart = 0; qStart < nq; qStart += BATCH_QUERY) {

    int qEnd = qStart + BATCH_QUERY;
    if (qEnd > nq) qEnd = nq;

    // CACHE BLOCKING SU DATASET
    for (int idxStart = 0; idxStart < N; idxStart += BLOCK_SIZE) {
      int idxEnd = idxStart + BLOCK_SIZE;
      if (idxEnd > N) idxEnd = N;

      // utilizziamo il blocco del dataset per ogni query nel batch
      for (int q = qStart; q < qEnd; q++) {

        VECTOR query = &input->Q[q * D];
        uint32_t* qPlus = &qPlusAll[q * num_blocchi_global];
        uint32_t* qMinus = &qMinusAll[q * num_blocchi_global];
        VECTOR dQP = &dQPAll[q * h];
        VECTOR current_KNN = &KNNAll[q * 2 * k];

        process_block_for_query(idxStart, idxEnd, query, input, qPlus, qMinus, dQP, current_KNN);
      }
    }
  }

  for (int q = 0; q < nq; q++)
  {
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
  for(int q = 0; q < nq; q++) {
    VECTOR query = &input->Q[q * D];
    VECTOR current_KNN = &KNNAll[q * 2 * k];
    // Calcola distanze euclidee reali per i k candidati rimasti
    for (int i = 0; i < k; i++)
    {
      int id_vicino = (int)current_KNN[2 * i];

      if (id_vicino >= 0)
      {
        VECTOR v = &input->DS[id_vicino * D];
        current_KNN[2 * i + 1] = dEuclidea(query, v, D);
      }
    }
    // Scrittura output finale
    for (int j = 0; j < k; j++)
    {
      input->id_nn[q * k + j] = (int)current_KNN[2 * j];
      input->dist_nn[q * k + j] = current_KNN[2 * j + 1];
    }
  }

  // Pulizia
  free(KNNAll);
  free(qPlusAll);
  free(qMinusAll);
  free(dQPAll);
  freePreQuantization();
}
