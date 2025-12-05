#include <stdlib.h>
#include <xmmintrin.h>
#include <omp.h>
#include "common.h"
#include <math.h>
#include <stdio.h>

const int INDEXING_PROCEDURE_ERROR = -1;
const int BLOCK_SIZE = 512;

int numeroBlocchiBitPacking = 0;
const int dimensioneSottovettoreBitPacking = 64;

// Variabili globali per i dati quantizzati
VECTOR vPlus_all = NULL;
VECTOR vMinus_all = NULL;
VECTOR pPlus = NULL;
VECTOR pMinus = NULL;


/* Ma che vuol dire che in C non c'è l'overloading della funzioni... -> https://en.cppreference.com/w/c/language/generic.html*/
extern float prodScalaref(float *v, float *w, int D);
extern double prodScalared(double *v, double *w, int D);
#define prodScalare(v,w,D) _Generic((v), float*: prodScalaref, double*:prodScalared)(v,w,D) 
extern float dEuclideaf(float *v, float *w, int D);
extern double dEuclidead(double *v, double *w, int D);
#define dEuclidea(v,w,D) _Generic((v), float*: dEuclideaf, double*:dEuclidead)(v,w,D) 

/*GENERALIZZO IL PROGRAMMA PER FUNZIONARE SIA CON DOUBLE CHE CON FLOAT*/
#define ABS(x) _Generic((x), float: fabsf, double: fabs)(x)

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
type distanzaApprossimataPreQ(__uint64_t* vPlus, __uint64_t* vMinus, __uint64_t* wPlus, __uint64_t* wMinus)
{
  type posPos = andBitABit(vPlus, wPlus, numeroBlocchiBitPacking);
  type negNeg = andBitABit(vMinus, wMinus, numeroBlocchiBitPacking);
  type posNeg = andBitABit(vPlus, wMinus, numeroBlocchiBitPacking);
  type negPos = andBitABit(vMinus, wPlus, numeroBlocchiBitPacking);
  
  return posPos + negNeg - posNeg - negPos;
}

// Costruzione indice (distanze dataset <-> pivot)
VECTOR indexing(params* input)
{
  int N = input->N;
  int h = input->h;

  VECTOR output = _mm_malloc(N * h * sizeof(type), align); 

  if (output == NULL) return NULL;

  for (int r = 0; r < N; r++)
  {
    __uint64_t* vPlus = &vPlus_all[r * numeroBlocchiBitPacking]; 
    __uint64_t* vMinus = &vMinus_all[r * numeroBlocchiBitPacking];
    for (int c = 0; c < h; c++)
    {
      __uint64_t* pPlusC = &pPlus[c * numeroBlocchiBitPacking];
      __uint64_t* pMinusC = &pMinus[c * numeroBlocchiBitPacking];
      output[r * h + c] = distanzaApprossimataPreQ(vPlus, vMinus, pPlusC, pMinusC);
    }
  }
  return output;
}

// Funzione di quantizzazione (versione HEAD)
void quantizing(VECTOR v, __uint64_t* vMinus, __uint64_t* vPlus, params* input, int *array_indici, VECTOR vMinus_Cuscinetto, VECTOR vPlus_Cuscinetto)
{
  int D = input->D;
  int x = input->x;

  // N.B. array_indici viene passato dall'esterno per evitare allocazioni ripetute

  // 1. Reset e Inizializzazione
  for (int k = 0; k < D; k++)
  {
    array_indici[k] = k;
    vPlus_Cuscinetto[k] = 0.0;
    vMinus_Cuscinetto[k] = 0.0;
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

    // Se v >= 0 -> vPlus=1, altrimenti vMinus=1
    if (v[original_idx] >= 0) 
    {
      vPlus_Cuscinetto[original_idx] = 1.0;
    }
    else 
    {
      vMinus_Cuscinetto[original_idx] = 1.0;
    }
  }

for (int i = 0; i < numeroBlocchiBitPacking; i++)
{
    for (int j = 0; j < dimensioneSottovettoreBitPacking; j++)  // ogni blocco ha max 64 bit
    {
        int idx = i * dimensioneSottovettoreBitPacking + j;
        if (idx >= D) break;  // evita di andare oltre D

        if (vPlus_Cuscinetto[idx] == 1.0)
            vPlus[i] |= 1ULL << j;   // imposta il bit j del blocco i

        if (vMinus_Cuscinetto[idx] == 1.0)
            vMinus[i] |= 1ULL << j;  // imposta il bit j del blocco i
    }
}

  
}

// Selezione Pivot
// FIXME: qui ho un dubbio, devo mettere VECTOR al dataset o MATRIX?
int *calcoloPivot(VECTOR dataSet, int h, int N, int D)
{
  printf("INIZIO CALCOLO PIVOT\n");
  int *pivot = (int *)_mm_malloc(h * sizeof(int), align); 

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
  vPlus_all = malloc((size_t)N * numeroBlocchiBitPacking * sizeof(__uint64_t));
  vMinus_all = malloc((size_t)N * numeroBlocchiBitPacking * sizeof(__uint64_t));

  if (!vPlus_all || !vMinus_all) {
    fprintf(stderr, "Errore allocazione vPlus_all/vMinus_all\n");
    if (vPlus_all) free(vPlus_all);
    if (vMinus_all) free(vMinus_all);
    vPlus_all = vMinus_all = NULL;
    return;
  }

  int *idx_buff = malloc(D * sizeof(int));
  VECTOR vPlus_Cuscinetto = calloc(D, sizeof(type));
  VECTOR vMinus_Cuscinetto = calloc(D, sizeof(type));

  for (int i = 0; i < N; i++)
  {
    VECTOR v = &input->DS[i * D]; // Vettore corrente del dataset
    __uint64_t* vp = &vPlus_all[i * numeroBlocchiBitPacking]; // Vettore vPlus da quantizzare, preso dalla matrice globale
    __uint64_t* vm = &vMinus_all[i * numeroBlocchiBitPacking]; // Vettore vMinus da quantizzare, preso dalla matrice globale
    quantizing(v, vm, vp, input, idx_buff, vMinus_Cuscinetto, vPlus_Cuscinetto);
  }

  free(idx_buff);
  free(vPlus_Cuscinetto);
  free(vMinus_Cuscinetto);
}

// Pre-quantizzazione dei Pivot
void preQuantizePivots(params *input)
{
  int D = input->D;
  int h = input->h;

  pPlus  = malloc((size_t)h * D * sizeof(type));
  pMinus = malloc((size_t)h * D * sizeof(type));

  if (!pPlus || !pMinus) {
    fprintf(stderr, "Errore allocazione pPlus/pMinus\n");
    if (pPlus) free(pPlus);
    if (pMinus) free(pMinus);
    pPlus = pMinus = NULL;
    return;
  }

  int *idx_buff = malloc(D * sizeof(int));
  VECTOR vPlus_Cuscinetto = calloc(D, sizeof(type));
  VECTOR vMinus_Cuscinetto = calloc(D, sizeof(type));

  for (int i = 0; i < h; i++)
  {
    int pivot_idx = input->P[i];
    if (pivot_idx < 0 || pivot_idx >= input->N) {
      // Gestione errore pivot fuori range
      VECTOR pp = &pPlus[i * numeroBlocchiBitPacking];
      VECTOR pm = &pMinus[i * numeroBlocchiBitPacking];
      for (int t = 0; t < D; t++) { pp[t]=0.0; pm[t]=0.0; }
      continue;
    }
    VECTOR p = &input->DS[pivot_idx * D];
    __uint64_t* pp = &pPlus[i * numeroBlocchiBitPacking];
    __uint64_t* pm = &pMinus[i * numeroBlocchiBitPacking];
    quantizing(p, pm, pp, input, idx_buff, vMinus_Cuscinetto, vPlus_Cuscinetto);
  }

  free(idx_buff);
  free(vPlus_Cuscinetto);
  free(vMinus_Cuscinetto);
}

// Processa un blocco di dataset [start_N, end_N) per una specifica query
void process_block_for_query(int start_N, int end_N, VECTOR query, params *input, VECTOR qPlus, VECTOR qMinus, VECTOR dQP, VECTOR KNN) 
{
  int D = input->D;
  int h = input->h;
  int k = input->k;


  // Itera SOLO sul blocco corrente del dataset
  for (int i = start_N; i < end_N; i++)
  {
    // A. Calcolo Lower Bound coi Pivot 
    type best_lb = 0.0;

    VECTOR current_index_row = &input->index[i * h];

    for (int j = 0; j < h; j++)
    {
      type d_vi_pj = current_index_row[j]; 
      type lb = ABS(d_vi_pj - dQP[j]);
      if (lb > best_lb)
        best_lb = lb;
    }

    type d_k_max = get_d_k_max(KNN, k);

    if (best_lb >= d_k_max)
      continue;

    __uint64_t* vPlus = &vPlus_all[i * numeroBlocchiBitPacking];
    __uint64_t* vMinus = &vMinus_all[i * numeroBlocchiBitPacking];
    type d_q_v_approx = distanzaApprossimataPreQ(qPlus, qMinus, vPlus, vMinus);

    // D. Inserimento
    if (d_q_v_approx < d_k_max)
    {
      insert_into_knn(KNN, k, i, d_q_v_approx);
    }
  }
}
void fit(params* input){
  
  numeroBlocchiBitPacking = ((input->D + 63) / 64); // arrotondamento a multiplo di 64

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
  __uint64_t* qPlusAll = malloc(numeroBlocchiBitPacking * nq * sizeof(__uint64_t));
  __uint64_t* qMinusAll = malloc(numeroBlocchiBitPacking * nq * sizeof(__uint64_t));
  VECTOR dQPAll = malloc(h * nq * sizeof(type));

  VECTOR KNNAll = malloc(2*k * nq *  sizeof(type));

  int *idx_buff = malloc(D * sizeof(int)); 
  VECTOR qPlus_Cuscinetto = calloc(D, sizeof(type));
  VECTOR qMinus_Cuscinetto = calloc(D, sizeof(type));
  for(int q = 0; q < nq; q++) {
    VECTOR query = &input->Q[q * D];
    __uint64_t* qPlus = &qPlusAll[q * numeroBlocchiBitPacking];
    __uint64_t* qMinus = &qMinusAll[q * numeroBlocchiBitPacking];
    VECTOR dQP = &dQPAll[q * h];
    VECTOR currentKNN = &KNNAll[q * 2 * k];

    // Init KNN a infinito
    for (int i = 0; i < k; i++)
    {
      currentKNN[2 * i] = -1.0f;     // ID
      currentKNN[2 * i + 1] = INFINITY; // Distanza
    }

    // Quantizza query
    quantizing(query, qMinus, qPlus, input, idx_buff, qMinus_Cuscinetto, qPlus_Cuscinetto);

    // Precalcola distanze query-pivot
    for (int j = 0; j < h; j++) {
      VECTOR pPlusC = &pPlus[j * D]; 
      VECTOR pMinusC = &pMinus[j * D];
      dQP[j] = distanzaApprossimataPreQ(qPlus, qMinus, pPlusC, pMinusC);
    }
  } 
  free(idx_buff);
  free(qPlus_Cuscinetto);
  free(qMinus_Cuscinetto);

  // iteriamo i blocchi di dataset in ram e calcoliamo tutte le query
#pragma omp parallel for
  for (int idxStart = 0; idxStart < N; idxStart += BLOCK_SIZE) {
    int idxEnd = idxStart + BLOCK_SIZE;
    if(idxEnd > N) idxEnd = N;
    for(int q = 0; q < nq; q++) {
      VECTOR query = &input->Q[q * D]; // Non usata in approximate, ma serve per firma
      VECTOR qPlus = &qPlusAll[q * D];
      VECTOR qMinus = &qMinusAll[q * D];
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
