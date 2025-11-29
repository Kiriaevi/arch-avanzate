#include <stdlib.h>
#include <xmmintrin.h>
#include <omp.h>
#include "common.h"
#include <math.h>
#include <stdio.h>
#define FLT_MAX 3.402823466e+38F

const int INDEXING_PROCEDURE_ERROR = -1;

// Variabili globali per i dati quantizzati
float *vPlus_all = NULL;
float *vMinus_all = NULL;
float *pPlus = NULL;
float *pMinus = NULL;

extern float prodScalare(float *v, float *w, int D);
extern float dEuclidea(float *v, float *w, int D);

// Funzione di pulizia (chiamata alla fine di predict)
void freePreQuantization()
{
  if (vPlus_all) { free(vPlus_all); vPlus_all = NULL; }
  if (vMinus_all) { free(vMinus_all); vMinus_all = NULL; }
  if (pPlus) { free(pPlus); pPlus = NULL; }
  if (pMinus) { free(pMinus); pMinus = NULL; }
}

// Gestione lista ordinata K-NN
void insert_into_knn(float *KNN, int k, int id, float distance)
{
  float max_distance = -1.0f;
  int max_index_id = -1;

  // Trova il vicino più lontano attualmente in lista (il candidato ad uscire)
  for (int i = 0; i < k; i++)
  {
    int index_dist = (i * 2) + 1;
    float current_distance = KNN[index_dist];

    if (current_distance > max_distance)
    {
      max_distance = current_distance;
      max_index_id = i * 2;
    }
  }

  // Se la nuova distanza è minore del peggiore attuale, sostituisci
  if (distance < max_distance)
  {
    KNN[max_index_id] = (float)id;
    KNN[max_index_id + 1] = distance;
  }
}

// Recupera la distanza massima attuale nella lista K-NN (il raggio di ricerca)
float get_d_k_max(float *KNN, int k)
{
  float max_distance = -1.0f;
  for (int i = 0; i < k; i++)
  {
    float current_distance = KNN[(i * 2) + 1];
    if (current_distance > max_distance)
    {
      max_distance = current_distance;
    }
  }
  return max_distance;
}

// Calcolo distanza approssimata (Eq. 2 del documento)
float distanzaApprossimataPreQ(float *vPlus, float *vMinus, float *wPlus, float *wMinus, int D)
{
  float posPos = prodScalare(vPlus, wPlus, D);
  float negNeg = prodScalare(vMinus, wMinus, D);
  float posNeg = prodScalare(vPlus, wMinus, D);
  float negPos = prodScalare(vMinus, wPlus, D);
  return posPos + negNeg - posNeg - negPos;
}

// Costruzione indice (distanze dataset <-> pivot)
float *indexing(params* input)
{
  int N = input->N;
  int h = input->h;

  // Uso _mm_malloc per allineamento (SSE=16, AVX=32)
  float *output = _mm_malloc(N * h * sizeof(float), 16); 
  
  if (output == NULL) return NULL;

  for (int r = 0; r < N; r++)
  {
    float *vPlus = &vPlus_all[r * input->D]; 
    float *vMinus = &vMinus_all[r * input->D];
    for (int c = 0; c < h; c++)
    {
      float *pPlusC = &pPlus[c * input->D];
      float *pMinusC = &pMinus[c * input->D];
      output[r * h + c] = distanzaApprossimataPreQ(vPlus, vMinus, pPlusC, pMinusC, input->D);
    }
  }
  return output;
}

// Funzione di quantizzazione (versione HEAD)
void quantizing(float *v, float *vMinus, float *vPlus, params* input, int *array_indici)
{
  int D = input->D;
  int x = input->x;
  
  // N.B. array_indici viene passato dall'esterno per evitare allocazioni ripetute

  // 1. Reset e Inizializzazione
  for (int k = 0; k < D; k++)
  {
    array_indici[k] = k;
    vPlus[k] = 0;
    vMinus[k] = 0;
  }

  // 2. Cerco gli X elementi con valore assoluto massimo (Partial Selection Sort)
  for (int i = 0; i < x; i++)
  {
    int maxIndex = i;
    float maxVal = fabsf(v[array_indici[i]]); 

    for (int j = i + 1; j < D; j++)
    {
      float currentVal = fabsf(v[array_indici[j]]);

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
      vPlus[original_idx] = 1.0f;
    }
    else 
    {
      vMinus[original_idx] = 1.0f;
    }
  }
}

// Selezione Pivot
int *calcoloPivot(float *dataSet, int h, int N, int D)
{
  printf("INIZIO CALCOLO PIVOT\n");
  int *pivot = (int *)_mm_malloc(h * sizeof(int), 16); 
  
  if (!pivot) return NULL; 

  int offset = (int)floorf((float)N / h);
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
  vPlus_all = malloc((size_t)N * D * sizeof(float));
  vMinus_all = malloc((size_t)N * D * sizeof(float));

  if (!vPlus_all || !vMinus_all) {
    fprintf(stderr, "Errore allocazione vPlus_all/vMinus_all\n");
    if (vPlus_all) free(vPlus_all);
    if (vMinus_all) free(vMinus_all);
    vPlus_all = vMinus_all = NULL;
    return;
  }

  // Buffer temporaneo per gli indici (richiesto dalla firma di quantizing)
  int *idx_buff = malloc(D * sizeof(int));

  for (int i = 0; i < N; i++)
  {
    float *v = &input->DS[i * D];
    float *vp = &vPlus_all[i * D];
    float *vm = &vMinus_all[i * D];
    quantizing(v, vm, vp, input, idx_buff);
  }
  
  free(idx_buff);
}

// Pre-quantizzazione dei Pivot
void preQuantizePivots(params *input)
{
  int D = input->D;
  int h = input->h;

  pPlus  = malloc((size_t)h * D * sizeof(float));
  pMinus = malloc((size_t)h * D * sizeof(float));

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
    if (pivot_idx < 0 || pivot_idx >= input->N) {
      // Gestione errore pivot fuori range
      float *pp = &pPlus[i * D];
      float *pm = &pMinus[i * D];
      for (int t = 0; t < D; t++) { pp[t]=0.0f; pm[t]=0.0f; }
      continue;
    }
    float *p = &input->DS[pivot_idx * D];
    float *pp = &pPlus[i * D];
    float *pm = &pMinus[i * D];
    quantizing(p, pm, pp, input, idx_buff);
  }
  
  free(idx_buff);
}

// Funzione di ricerca per singola query
// NOTA: Firma aggiornata per accettare buffer esterni (qPlus, qMinus, etc.)
float *querying2(float *query, params *input, float *qPlus, float *qMinus, float *dQP, int *array_indici)
{
  int D = input->D;
  int h = input->h;
  int k = input->k;
  int N = input->N;

  // 1. Quantizza la query
  quantizing(query, qMinus, qPlus, input, array_indici);

  // 2. Calcola d(q,p) per ogni pivot 
  for (int j = 0; j < h; j++)
  {
    float *pPlusC = &pPlus[j * D]; 
    float *pMinusC = &pMinus[j * D];
    dQP[j] = distanzaApprossimataPreQ(qPlus, qMinus, pPlusC, pMinusC, D);
  }

  // 3. Inizializza K-NN 
  int dim = 2 * k;
  float *KNN = malloc(dim * sizeof(float));
  for (int i = 0; i < k; i++)
  {
    KNN[2 * i] = -1.0f;     // ID
    KNN[2 * i + 1] = FLT_MAX; // Distanza
  }

  // 4. Itera sul dataset 
  for (int i = 0; i < N; i++)
  {
    // A. Calcolo Lower Bound coi Pivot 
    float best_lb = 0.0f;
    for (int j = 0; j < h; j++)
    {
      float d_vi_pj = input->index[i * h + j]; // Accesso all'indice precalcolato
      float lb = fabsf(d_vi_pj - dQP[j]);
      if (lb > best_lb)
        best_lb = lb;
    }

    // Recupera la distanza massima attuale nel K-NN
    float d_k_max = get_d_k_max(KNN, k);

    // B. Filtro Pivot 
    if (best_lb >= d_k_max)
      continue;

    // C. Calcolo Distanza Approssimata 
    float *vPlus = &vPlus_all[i * D];
    float *vMinus = &vMinus_all[i * D];
    float d_q_v_approx = distanzaApprossimataPreQ(qPlus, qMinus, vPlus, vMinus, D);

    // D. Inserimento basato su Distanza Approssimata 
    if (d_q_v_approx < d_k_max)
    {
      insert_into_knn(KNN, k, i, d_q_v_approx);
    }
  }

  // 5. Raffinamento Finale (Ricalcolo Euclideo - LOGICA HEAD)
  // Solo ora calcoliamo la distanza reale per i vincitori
  for (int i = 0; i < k; i++)
  {
     int id_vicino = (int)KNN[2 * i];
     
     if (id_vicino >= 0) {
         float *v = &input->DS[id_vicino * D];
         // Sostituiamo la distanza approssimata con quella reale
         KNN[2 * i + 1] = dEuclidea(query, v, D); 
     }
  }

  return KNN;
}

void fit(params* input){
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
  int D = input->D;
  int k = input->k;
  int h = input->h;

  // Allocazione buffer temporanei FUORI dal ciclo per performance
  // Questo abilita la logica usata in querying2
  float *qPlus = malloc(D * sizeof(float));
  float *qMinus = malloc(D * sizeof(float));
  float *dQP = malloc(h * sizeof(float));
  int *array_indici = malloc(D * sizeof(int));

  for(int i = 0; i < nq; i++) {
    float *query = &input->Q[i*D];
    
    // Passiamo i buffer allocati
    float *KNN = querying2(query, input, qPlus, qMinus, dQP, array_indici);

    for (int j = 0; j < k; j++) {
      input->id_nn[i*k + j] = (int) KNN[2*j];      
      input->dist_nn[i*k + j] = KNN[2*j + 1];     
    }

    free(KNN); 
  }

  // Pulizia buffer temporanei
  free(qPlus);
  free(qMinus);
  free(dQP);
  free(array_indici);

  // Pulizia dati quantizzati globali
  freePreQuantization();
}
