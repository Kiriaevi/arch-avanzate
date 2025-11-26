#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

static int N = 2000; // Righe dataset
static int D = 256;  // Colonne dataset
static int h = 16;   // numero di pivot
static int x = 64;   // parametro di quantizzazione
static int k = 8;    // numero di vicini

extern float prodScalare(float *v, float *w, int D);

// definisco i vettori in modo globale
float *vPlus_all = NULL;
float *vMinus_all = NULL;
float *pPlus = NULL;
float *pMinus = NULL;

#define FLT_MAX 3.402823466e+38F

//
// ---------------------------------------------------------------
//  PROTOTIPI
// ---------------------------------------------------------------
float *readFile(char *f);
float *calcoloPivot(float *dataSet, int h, int N, int D);
float *executeQuery(float q);

// Aggiornato: firma per buffer esterni
float distanzaApprossimata(float *v, float *w, float *vMinus, float *vPlus, float *wMinus, float *wPlus);

float *indexing(float *p, float *v);
float dEuclidea(float *v, float *w);
// float prodScalare(float *v, float *w);

void quantizing(float *v, float *vMinus, float *vPlus);

void testQuantizing(float *v, const char *name);
int nomeFittizio();
void testIndexing();
void testDistanzaApprossimata();
void testQueryingCompleto();

float *querying(float *query, float *pivot, float *dataSet, float *vettoreIndexing);

void preQuantizeDataset(float *dataset);

//
// ---------------------------------------------------------------
//  LETTURA FILE (placeholder)
// ---------------------------------------------------------------
float *readFile(char *f)
{
  return NULL;
}

//
// ---------------------------------------------------------------
//  CALCOLO PIVOT
// ---------------------------------------------------------------
float *calcoloPivot(float *dataSet, int h, int N, int D)
{
  float *pivot = (float *)malloc(sizeof(float) * D * h);
  int offset = (int)floorf((float)N / h);
  int k = 0;

  for (int i = 0; i < N && k < h; i += offset)
  {
    for (int j = 0; j < D; j++)
    {
      pivot[k * D + j] = dataSet[i * D + j];
    }
    k++;
  }
  return pivot;
}

//
// ---------------------------------------------------------------
//  QUERY (placeholder)
// ---------------------------------------------------------------
float *executeQuery(float q)
{
  return NULL;
}

//
// ---------------------------------------------------------------
//  DISTANZA EUCLIDEA
// ---------------------------------------------------------------
float dEuclidea(float *v, float *w)
{
  float distanza = 0.0f;
  for (int i = 0; i < D; i++)
  {
    float diff = v[i] - w[i];
    distanza += diff * diff;
  }
  return sqrtf(distanza);
}

//
// ---------------------------------------------------------------
//  PRODOTTO SCALARE
// ---------------------------------------------------------------
// float prodScalare(float *v, float *w) {
//    float prod = 0.0f;
//    for (int i = 0; i < D; i++) {
//        prod += v[i] * w[i];
//    }
//    return prod;
//}

//
// ---------------------------------------------------------------
//  QUANTIZZAZIONE
// ---------------------------------------------------------------
void quantizing(float *v, float *vMinus, float *vPlus)
{
  int array_indici[D];

  // Reset dei vettori output e inizializzazione indici
  for (int k = 0; k < D; k++)
  {
    array_indici[k] = k;
    vPlus[k] = 0;
    vMinus[k] = 0;
  }

  // cerco X massimi
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

    int temp = array_indici[i];
    array_indici[i] = array_indici[maxIndex];
    array_indici[maxIndex] = temp;
  }

  // Gestione segno per i top X
  for (int i = 0; i < x; i++)
  {
    int idx = array_indici[i];
    if (v[idx] < 0)
    {
      array_indici[i] = -idx;
    }
  }

  // Assegno ai vettori plus/minus
  for (int i = 0; i < x; i++)
  {
    int idx = array_indici[i];
    if (idx < 0)
      vMinus[-idx] = 1;
    else
      vPlus[idx] = 1;
  }
}

//
// ---------------------------------------------------------------
//  DISTANZA APPROSSIMATA (CORRETTA)
// ---------------------------------------------------------------
float distanzaApprossimata(float *v, float *w, float *vMinus, float *vPlus, float *wMinus, float *wPlus)
{

  // === PUNTO CRITICO CORRETTO ===
  // I buffer vengono passati allocati ma vuoti (o sporchi).
  // Dobbiamo chiamare quantizing qui per riempirli con i dati correnti di v e w.
  quantizing(v, vMinus, vPlus);
  quantizing(w, wMinus, wPlus);
  // ==============================

  float posPos = prodScalare(vPlus, wPlus, D);
  float negNeg = prodScalare(vMinus, wMinus, D);
  float posNeg = prodScalare(vPlus, wMinus, D);
  float negPos = prodScalare(vMinus, wPlus, D);

  float approx = posPos + negNeg - posNeg - negPos;

  return approx;
}

float *estraiRiga(float *v, int idx)
{
  float *output = malloc(D * sizeof(float));
  for (int j = 0; j < D; j++)
  {
    output[j] = v[idx + j];
  }
  return output;
}

//
// ---------------------------------------------------------------
//  INDEXING
// ---------------------------------------------------------------
/*
float* indexing(float *p, float *v) {
    float *output = malloc(N * h * sizeof(float));

    // Allocazione buffer una tantum
    float *buf_vMinus = malloc(D * sizeof(float));
    float *buf_vPlus  = malloc(D * sizeof(float));
    float *buf_wMinus = malloc(D * sizeof(float));
    float *buf_wPlus  = malloc(D * sizeof(float));

    for (int r = 0; r < N; r++) {
        float *vettore = &v[r * D];
        for (int c = 0; c < h; c++) {
            float *vPivot = &p[c * D];
            // Passaggio buffer
            output[r * h + c] = distanzaApprossimata(vettore, vPivot, buf_vMinus, buf_vPlus, buf_wMinus, buf_wPlus);
        }
    }

    free(buf_vMinus);
    free(buf_vPlus);
    free(buf_wMinus);
    free(buf_wPlus);
    return output;
}

*/

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

void insert_into_knn(float *KNN, int k, int id, float distance)
{
  float max_distance = -1.0f;
  int max_index_id = -1;

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

  if (distance < max_distance)
  {
    KNN[max_index_id] = (float)id;
    KNN[max_index_id + 1] = distance;
  }
}

//
// ---------------------------------------------------------------
//  QUERYING
// ---------------------------------------------------------------
float *querying(float *query, float *pivot, float *dataSet, float *vettoreIndexing)
{
  int dimensioneKnn = 2 * k;
  float *KNN = malloc(sizeof(float) * dimensioneKnn);

  for (int i = 0; i < k; i++)
  {
    KNN[i * 2] = -1;
    KNN[i * 2 + 1] = FLT_MAX;
  }

  // Allocazione buffer una tantum
  float *buf_vMinus = malloc(D * sizeof(float));
  float *buf_vPlus = malloc(D * sizeof(float));
  float *buf_wMinus = malloc(D * sizeof(float));
  float *buf_wPlus = malloc(D * sizeof(float));

  // Calcola le distanze approssimate query -> pivot
  float *distanzaApprossimataQP = malloc(sizeof(float) * h);

  for (int i = 0; i < h; i++)
  {
    float *pivotCuscinetto = &pivot[i * D];
    distanzaApprossimataQP[i] = distanzaApprossimata(query, pivotCuscinetto, buf_vMinus, buf_vPlus, buf_wMinus, buf_wPlus);
  }

  // Scorri tutti i vettori del dataset
  for (int i = 0; i < N; i++)
  {
    float *vettore = &dataSet[i * D];
    float d_pvt_max = 0.0f;

    // Calcola lower-bound tramite pivot
    for (int j = 0; j < h; j++)
    {
      float d_vi_pj = vettoreIndexing[i * h + j];
      float d_q_pj = distanzaApprossimataQP[j];
      float limiteInferiore = fabsf(d_vi_pj - d_q_pj);
      if (limiteInferiore > d_pvt_max)
        d_pvt_max = limiteInferiore;
    }

    float d_k_max = get_d_k_max(KNN, k);

    if (d_pvt_max < d_k_max)
    {
      // SECONDO FILTRO
      float d_q_v_approx = distanzaApprossimata(query, vettore, buf_vMinus, buf_vPlus, buf_wMinus, buf_wPlus);

      if (d_q_v_approx < d_k_max)
      {
        // TERZO FILTRO: Distanza reale
        float d_q_v_reale = dEuclidea(query, vettore);

        if (d_q_v_reale < d_k_max)
        {
          insert_into_knn(KNN, k, i, d_q_v_reale);
        }
      }
    }
  }

  free(distanzaApprossimataQP);
  free(buf_vMinus);
  free(buf_vPlus);
  free(buf_wMinus);
  free(buf_wPlus);
  return KNN;
}
void preQuantizeDataset(float *dataset)
{
  // Il suo scopo è quello di allocare due "matrici" memorizzate come array per contenere le versioni quantizzate di tutti i vettori del dataset
  vPlus_all = malloc(N * D * sizeof(float));
  vMinus_all = malloc(N * D * sizeof(float));
  for (int i = 0; i < N; i++)
  {
    // Per ogni vettore del dataset, lo quantizzo e lo aggiungo ai vettori dei quantizzati
    float *v = &dataset[i * D];
    float *vp = &vPlus_all[i * D];
    float *vm = &vMinus_all[i * D];
    quantizing(v, vm, vp);
  }
}
void preQuantizePivots(float *pivot)
{
  // Segue la stessa logica della funzione che prequantizza il dataset
  pPlus = malloc(h * D * sizeof(float));
  pMinus = malloc(h * D * sizeof(float));
  for (int i = 0; i < h; i++)
  {
    float *p = &pivot[i * D];
    float *pp = &pPlus[i * D];
    float *pm = &pMinus[i * D];
    quantizing(p, pm, pp);
  }
}
float distanzaApprossimataPreQ(float *vPlus, float *vMinus, float *wPlus, float *wMinus)
{
  float posPos = prodScalare(vPlus, wPlus, D);
  float negNeg = prodScalare(vMinus, wMinus, D);
  float posNeg = prodScalare(vPlus, wMinus, D);
  float negPos = prodScalare(vMinus, wPlus, D);
  return posPos + negNeg - posNeg - negPos;
}
float *indexing(float *pivot, float *dataset)
{
  // È necessario aver prequantizzato i pivot ed il dataset per usare questa funzione
  float *output = malloc(N * h * sizeof(float));
  for (int r = 0; r < N; r++)
  {
    float *vPlus = &vPlus_all[r * D];
    float *vMinus = &vMinus_all[r * D];
    for (int c = 0; c < h; c++)
    {
      float *pPlusC = &pPlus[c * D];
      float *pMinusC = &pMinus[c * D];
      output[r * h + c] = distanzaApprossimataPreQ(vPlus, vMinus, pPlusC, pMinusC);
    }
  }
  return output;
}
float *querying2(float *query, float *pivot, float *dataSet, float *vettoreIndexing)
{
  // quantizza la query
  float *qPlus = malloc(D * sizeof(float));
  float *qMinus = malloc(D * sizeof(float));
  quantizing(query, qMinus, qPlus);
  // calcola d(q,p) per ogni pivot
  float *dQP = malloc(h * sizeof(float));
  for (int j = 0; j < h; j++)
  {
    float *pPlusC = &pPlus[j * D];
    float *pMinusC = &pMinus[j * D];
    dQP[j] = distanzaApprossimataPreQ(qPlus, qMinus, pPlusC, pMinusC);
  }
  // Preparo il KNN
  int dim = 2 * k;
  float *KNN = malloc(dim * sizeof(float));
  for (int i = 0; i < k; i++)
  {
    KNN[2 * i] = -1;
    KNN[2 * i + 1] = FLT_MAX;
  }
  // Itero sul dataset
  for (int i = 0; i < N; i++)
  {
    float best_lb = 0.0f;
    for (int j = 0; j < h; j++)
    {
      float d_vi_pj = vettoreIndexing[i * h + j];
      float lb = fabsf(d_vi_pj - dQP[j]);
      if (lb > best_lb)
        best_lb = lb;
    }
    float d_k_max = get_d_k_max(KNN, k);
    if (best_lb >= d_k_max)
      continue;
    float *vPlus = &vPlus_all[i * D];
    float *vMinus = &vMinus_all[i * D];
    float d_q_v_approx = distanzaApprossimataPreQ(qPlus, qMinus, vPlus, vMinus);
    if (d_q_v_approx >= d_k_max)
      continue; // non considero il vettore

    // Calcolo distanza reale
    float *v = &dataSet[i * D];
    float d_q_v_real = dEuclidea(query, v);

    if (d_q_v_real < d_k_max)
    {
      insert_into_knn(KNN, k, i, d_q_v_real);
    }
  }
  free(qPlus);
  free(qMinus);
  free(dQP);
  return KNN;
}

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

//
// ---------------------------------------------------------------
//  FUNZIONI DI TEST
// ---------------------------------------------------------------
void testQuantizing(float *v, const char *name)
{

  printf("\n\n===== TEST %s =====\n", name);

  float *vPlus = calloc(D, sizeof(float));
  float *vMinus = calloc(D, sizeof(float));

  quantizing(v, vMinus, vPlus);

  printf("\nv: ");
  for (int i = 0; i < D; i++)
    printf("%.1f ", v[i]);

  printf("\nvPlus:  ");
  for (int i = 0; i < D; i++)
    printf("%.0f ", vPlus[i]);

  printf("\nvMinus: ");
  for (int i = 0; i < D; i++)
    printf("%.0f ", vMinus[i]);

  printf("\n");

  free(vPlus);
  free(vMinus);
}

int nomeFittizio()
{
  float test1[] = {2, -6, 1, -5, 3, 4};
  testQuantizing(test1, "Valori misti");
  return 0;
}

void testDistanzaApprossimata()
{
  printf("\n===== TEST distanzaApprossimata =====\n");

  float v[D], w[D];
  for (int i = 0; i < D; i++)
  {
    v[i] = ((float)rand() / RAND_MAX) * 2 - 1;
    w[i] = ((float)rand() / RAND_MAX) * 2 - 1;
  }

  // Buffer temporanei per il test
  float *b1 = malloc(D * sizeof(float));
  float *b2 = malloc(D * sizeof(float));
  float *b3 = malloc(D * sizeof(float));
  float *b4 = malloc(D * sizeof(float));

  float reale = dEuclidea(v, w);
  float approx = distanzaApprossimata(v, w, b1, b2, b3, b4);

  printf("Distanza reale    = %.4f\n", reale);
  printf("Distanza approx   = %.4f\n", approx);

  free(b1);
  free(b2);
  free(b3);
  free(b4);
}

void testIndexing()
{
  printf("\n===== TEST indexing =====\n");

  float *dataset = malloc(N * D * sizeof(float));
  float *pivots = malloc(h * D * sizeof(float));

  // dataset casuale
  for (int i = 0; i < N; i++)
    for (int j = 0; j < D; j++)
      dataset[i * D + j] = ((float)rand() / RAND_MAX) * 20 - 10;
  ;

  // pivots
  float *pv = calcoloPivot(dataset, h, N, D);
  memcpy(pivots, pv, h * D * sizeof(float));
  free(pv);

  float *IDX_app = malloc(N * h * sizeof(float));
  float *IDX_real = malloc(N * h * sizeof(float));

  // Allocazione buffer test
  float *b_vMinus = malloc(D * sizeof(float));
  float *b_vPlus = malloc(D * sizeof(float));
  float *b_wMinus = malloc(D * sizeof(float));
  float *b_wPlus = malloc(D * sizeof(float));

  printf("\nConfronto distanze reali vs approssimate (primi 10):\n");
  int print_limit = 10;
  int print_count = 0;

  for (int r = 0; r < N; r++)
  {
    for (int c = 0; c < h; c++)
    {
      float *vettore = &dataset[r * D];
      float *vPivot = &pivots[c * D];

      IDX_real[r * h + c] = dEuclidea(vettore, vPivot);
      IDX_app[r * h + c] = distanzaApprossimata(vettore, vPivot, b_vMinus, b_vPlus, b_wMinus, b_wPlus);

      if (print_count < print_limit)
      {
        printf("Riga %d Pivot %d: Reale = %.4f, Approssimata = %.4f\n",
               r, c, IDX_real[r * h + c], IDX_app[r * h + c]);
        print_count++;
      }
    }
  }

  free(b_vMinus);
  free(b_vPlus);
  free(b_wMinus);
  free(b_wPlus);
  free(dataset);
  free(pivots);
  free(IDX_app);
  free(IDX_real);
}

void testQueryingCompleto()
{
  printf("\n===== TEST querying COMPLETO =====\n");

  // 1. Genera dataset casuale
  float *dataset = malloc(N * D *  sizeof(float));
  if (!dataset)
  {
    fprintf(stderr, "Errore allocazione dataset!\n");
    return;
  }

  for (int i = 0; i < N; i++)
    for (int j = 0; j < D; j++)
      dataset[i * D + j] = ((float)rand() / RAND_MAX) * 20 - 10;

  // 2. Calcola pivot e pre-quantizzazione
  float *pivot = calcoloPivot(dataset, h, N, D);
  if (!pivot)
  {
    fprintf(stderr, "Errore calcolo pivot!\n");
    free(dataset);
    return;
  }

  preQuantizeDataset(dataset);
  preQuantizePivots(pivot);

  // 3. Costruisci indice
  float *vettoreIndexing = indexing(pivot, dataset);
  if (!vettoreIndexing)
  {
    fprintf(stderr, "Errore indexing!\n");
    free(dataset);
    free(pivot);
    freePreQuantization();
    return;
  }

  // 4. Genera query casuale
  float *query = malloc(D * sizeof(float));
  if (!query)
  {
    fprintf(stderr, "Errore allocazione query!\n");
    free(dataset);
    free(pivot);
    free(vettoreIndexing);
    freePreQuantization();
    return;
  }

  for (int i = 0; i < D; i++)
    query[i] = ((float)rand() / RAND_MAX) * 20 - 10;

  // 5. Esegui KNN approssimato
  float *KNN = querying2(query, pivot, dataset, vettoreIndexing);
  if (!KNN)
  {
    fprintf(stderr, "Errore querying!\n");
    free(dataset);
    free(pivot);
    free(vettoreIndexing);
    free(query);
    freePreQuantization();
    return;
  }

  printf("\nK-NN trovati da querying (id, distanza reale):\n");
  for (int i = 0; i < k; i++)
  {
    int id = (int)KNN[i * 2];
    float dist = KNN[i * 2 + 1];
    if (id != -1)
      printf("%d: %.4f\n", id, dist);
  }

  // 6. Calcola distanze euclidee reali - USA UN ARRAY SEPARATO
  float *realDistances = malloc(N * sizeof(float));
  if (!realDistances)
  {
    fprintf(stderr, "Errore allocazione realDistances!\n");
    free(dataset);
    free(pivot);
    free(vettoreIndexing);
    free(KNN);
    free(query);
    freePreQuantization();
    return;
  }

  // CALCOLA TUTTE LE DISTANZE
  for (int i = 0; i < N; i++)
  {
    float *v = &dataset[i * D];
    realDistances[i] = dEuclidea(query, v);
  }

  // 7. Crea una COPIA per l'ordinamento
  float *sortedDistances = malloc(N * sizeof(float));
  if (!sortedDistances)
  {
    fprintf(stderr, "Errore allocazione sortedDistances!\n");
    free(realDistances);
    free(dataset);
    free(pivot);
    free(vettoreIndexing);
    free(KNN);
    free(query);
    freePreQuantization();
    return;
  }
  memcpy(sortedDistances, realDistances, N * sizeof(float));

  printf("\nK-NN reali (distanza euclidea):\n");
  for (int n = 0; n < k; n++)
  {
    int min_idx = -1;
    float min_val = FLT_MAX;
    for (int i = 0; i < N; i++)
    {
      if (sortedDistances[i] < min_val)
      {
        min_val = sortedDistances[i];
        min_idx = i;
      }
    }
    if (min_idx != -1)
    {
      printf("%d: %.4f\n", min_idx, min_val);
      sortedDistances[min_idx] = FLT_MAX;
    }
  }

  // 8. Verifica errori - USA realDistances ORIGINALE
  float max_dist_knn = 0.0f;
  for (int i = 0; i < k; i++)
    if (KNN[i * 2 + 1] > max_dist_knn)
      max_dist_knn = KNN[i * 2 + 1];

  int errori = 0;
  for (int i = 0; i < N; i++)
  {
    int in_knn = 0;
    for (int j = 0; j < k; j++)
      if ((int)KNN[j * 2] == i)
      {
        in_knn = 1;
        break;
      }

    if (!in_knn)
    {
      // USA realDistances che non è stato modificato!
      float dist = realDistances[i];
      if (dist < max_dist_knn)
      {
        printf("ERRORE: punto %d escluso ma distanza %.4f < %.4f\n",
               i, dist, max_dist_knn);
        errori++;
      }
    }
  }

  if (errori == 0)
    printf("✓ Nessun errore! Tutti i punti esclusi sono effettivamente più lontani.\n");
  else
    printf("✗ Trovati %d errori!\n", errori);

  // 9. Libera memoria
  free(sortedDistances);
  free(realDistances);
  free(dataset);
  free(pivot);
  free(vettoreIndexing);
  free(KNN);
  free(query);
  freePreQuantization();
}

//
// ---------------------------------------------------------------
//  MAIN
// ---------------------------------------------------------------
int main(int argc, char **argv)
{
  srand((unsigned)time(NULL));
  if (argc != 6)
  {
    printf("Inserisci numero di valori appropritato");
  }
  N = atoi(argv[1]);
  D = atoi(argv[2]);
  h = atoi(argv[3]);
  x = atoi(argv[4]);
  k = atoi(argv[5]);

  // nomeFittizio();
  // testDistanzaApprossimata();
  // testIndexing();
  testQueryingCompleto();
  return 0;
}
