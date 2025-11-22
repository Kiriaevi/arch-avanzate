#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

static int N = 10000; //Righe dataset
static int D = 2000; //Colonne dataset
static int h = 40; // numero di pivot
static int x = 30; // parametro di quantizzazione
static int k = 16; // numero di vicini

#define FLT_MAX 3.402823466e+38F

//
// ---------------------------------------------------------------
//  PROTOTIPI
// ---------------------------------------------------------------
float* readFile(char *f);
float* calcoloPivot(float *dataSet, int h, int N, int D);
float* executeQuery(float q);
float* indexing(float *p, float *v);

float dEuclidea(float *v, float *w);
float prodScalare(float *v, float *w);

int comparatore(const void *a, const void *b);
void quantizing(float *v, float *vMinus, float *vPlus);

void testQuantizing(float *v, const char *name);
int nomeFittizio();

float* querying(float *query, float *pivot, float *dataSet, float* vettoreIndexing);



//
// ---------------------------------------------------------------
//  LETTURA FILE (placeholder)
// ---------------------------------------------------------------
float* readFile(char *f) {
  return NULL;
}


//
// ---------------------------------------------------------------
//  CALCOLO PIVOT
// ---------------------------------------------------------------
float* calcoloPivot(float *dataSet, int h, int N, int D) {
  float* pivot = (float*)malloc(sizeof(float) * D * h);
  int offset = (int) floorf((float)N / h);
  int k = 0;

    for (int i = 0; i < N && k < h; i += offset) {
        for (int j = 0; j < D; j++) {
            pivot[k*D + j] = dataSet[i*D + j];
        }
        k++;
    }
  return pivot;
}


//
// ---------------------------------------------------------------
//  QUERY (placeholder)
// ---------------------------------------------------------------
float* executeQuery(float q) {
  return NULL;
} 


//
// ---------------------------------------------------------------
//  DISTANZA EUCLIDEA
// ---------------------------------------------------------------
float dEuclidea(float *v, float *w) {
    float distanza = 0.0f;
    for (int i = 0; i < D; i++) {
        float diff = v[i] - w[i];
        distanza += diff * diff;
    }
    return sqrtf(distanza);
}


//
// ---------------------------------------------------------------
//  PRODOTTO SCALARE
// ---------------------------------------------------------------
float prodScalare(float *v, float *w) {
    /*printf("Calcolo prodotto scalare fra \n");
    for (int i = 0; i < D; i++){
        printf("%.2f ",v[i]);
    }
    printf("\n");
    for (int i = 0; i < D; i++){
        printf("%.2f ",w[i]);
    }
    printf("\n");
    */
    float prod = 0.0f;
    for (int i = 0; i < D; i++) {
        prod += v[i] * w[i];
    }
    //printf("Risultato = %.5f \n",prod);
    return prod;
}


//
// ---------------------------------------------------------------
//  QUANTIZZAZIONE
// ---------------------------------------------------------------
static const float *arr_ctx;

int comparatore(const void *a, const void *b) {
    int i1 = *(int*)a;
    int i2 = *(int*)b;

    float v1 = fabsf(arr_ctx[i1]);
    float v2 = fabsf(arr_ctx[i2]);

    if (v1 < v2) return  1;
    if (v1 > v2) return -1;
    return 0;
}


void quantizing(float *v, float *vMinus, float *vPlus) {
    /*printf("\nQuantizing di: ");
        for (int i = 0; i < D; i++) 
            printf("%.2f ", v[i]);
            **/
    int array_indici[D];
    for (int i = 0; i < D; i++){
        array_indici[i] = i;
        vPlus[i] = 0; // azzero, per sicurezza
        vMinus[i] = 0; // azzero, per sicurezza
    }

    arr_ctx = v;
    qsort(array_indici, D, sizeof(int), comparatore);

    // applico segno
    for (int i = 0; i < D; i++)
        if (v[array_indici[i]] < 0)
            array_indici[i] = -array_indici[i];

   /* printf("\nOrdine indici assoluti con segno: ");
    for (int i = 0; i < D; i++)
        printf("%d ", array_indici[i]);
*/
    if (x > D) {
        fprintf(stderr, "ERRORE: richiesti %d massimi ma dataset ha dimensione %d\n", x, D);
        return;
    }

    // assegno ai vettori plus/minus
    for (int i = 0; i < x; i++) {
        int idx = array_indici[i];
        if (idx < 0)
            vMinus[-idx] = 1;
        else
            vPlus[idx] = 1;
    }

    arr_ctx = NULL;
/*
    printf("\nTop-%d elementi per |valore|: ", x);
    for (int k = 0; k < x; k++) 
        printf("%d ", array_indici[k]);

    printf("\nvPlus:  ");
    for (int i = 0; i < D; i++)
        printf("%.0f ", vPlus[i]);

    printf("\nvMinus: ");
    for (int i = 0; i < D; i++) 
        printf("%.0f ", vMinus[i]);

    printf("\n");
    */
}
float distanzaApprossimata(float *v, float *w) {
    // Alloco i vettori quantizzati
    float *vMinus = calloc(D, sizeof(float));
    float *vPlus  = calloc(D, sizeof(float));
    float *wMinus = calloc(D, sizeof(float));
    float *wPlus  = calloc(D, sizeof(float));
    // Quantizzo v e w
//    printf("\n Quantize del primo vettore\n");
    quantizing(v, vMinus, vPlus);
//    printf("\nQuantize del secondo vettore\n");
    quantizing(w, wMinus, wPlus);
    // Calcolo i prodotti scalari
  //  printf("Calcolo dei prodotti scalari dei due vettori\n");
    float posPos = prodScalare(vPlus, wPlus);
    float negNeg = prodScalare(vMinus, wMinus);
    float posNeg = prodScalare(vPlus, wMinus);
    float negPos = prodScalare(vMinus, wPlus);
    // Stampo tutti i termini
    //printf("\nDistanza approssimata:\n");
    //printf("v+ · w+ = %.5f\n", posPos);
    //printf("v- · w- = %.5f\n", negNeg);
    //printf("v+ · w- = %.5f\n", posNeg);
    //printf("v- · w+ = %.5f\n", negPos);
    float approx = posPos + negNeg - posNeg - negPos;
    //printf("Distanza approssimata = %.5f\n", approx);
    // Libero la memoria
    free(vMinus);
    free(vPlus);
    free(wMinus);
    free(wPlus);
    return approx;
}

float* estraiRiga(float *v, int idx){
    float* output = malloc(D*sizeof(float));
    for(int j = 0; j < D; j++) {
        output[j] = v[idx + j]; 
    }
    return output;
}
float* indexing(float *p, float *v) {
    float *output = malloc(N * h * sizeof(float));
    for (int r = 0; r < N; r++) {
        float *vettore = &v[r * D];   // CAMBIA QUI
        for (int c = 0; c < h; c++) {
            float *vPivot = &p[c * D]; // E QUI
            output[r * h + c] = distanzaApprossimata(vettore, vPivot);
        }
    }
    return output;
}

float get_d_k_max(float *KNN, int k) {
    // La lista KNN è [id1, d1, id2, d2, ..., idk, dk]. 
    // Le distanze si trovano agli indici dispari.
    float max_distance = -1.0f; // Inizializza con un valore negativo (distanza non negativa)
    
    // Itera su tutti i k punti (2*k elementi, ma solo k distanze)
    for (int i = 0; i < k; i++) {
        // L'indice della distanza del punto i-esimo è (i * 2) + 1
        float current_distance = KNN[(i * 2) + 1]; 
        
        // Controlla e aggiorna il massimo
        if (current_distance > max_distance) {
            max_distance = current_distance;
        }
    }
    return max_distance;
}
void insert_into_knn(float *KNN, int k, int id, float distance) {
    float max_distance = -1.0f;
    int max_index_id = -1; // Indice pari (posizione dell'ID) del punto più lontano

    // 1. Trova l'elemento più lontano (d_k_max) e la sua posizione
    // Scorriamo tutte le k distanze (agli indici dispari)
    for (int i = 0; i < k; i++) {
        int index_dist = (i * 2) + 1;
        float current_distance = KNN[index_dist];
        
        // Se troviamo una distanza maggiore della massima finora, la memorizziamo
        if (current_distance > max_distance) {
            max_distance = current_distance;
            max_index_id = i * 2; // Memorizziamo l'indice dell'ID
        }
    }

    // 2. Sostituzione (Se il nuovo punto è più vicino del punto più lontano attuale)
    if (distance < max_distance) {
        // La posizione dell'ID del punto più lontano è max_index_id
        // La posizione della DISTANZA è max_index_id + 1
        
        KNN[max_index_id] = (float)id;          // Inserisce il nuovo ID
        KNN[max_index_id + 1] = distance;      // Inserisce la nuova distanza
        
        // NOTA: Il tuo algoritmo di pruning richiede solo la sostituzione.
        // Per avere la lista *perfettamente* ordinata in ogni istante 
        // (che non è strettamente necessario per la sola get_d_k_max),
        // servirebbe un'operazione di riordino, ma il metodo attuale è il più efficiente.
    }
}
//
// ---------------------------------------------------------------
//  QUERYING
// ---------------------------------------------------------------
float* querying(float *query, float *pivot, float *dataSet, float* vettoreIndexing) {
    // KNN: [id1, d1, id2, d2, ..., idk, dk]
    int dimensioneKnn = 2 * k;
    float* KNN = malloc(sizeof(float) * dimensioneKnn);

    // Inizializza KNN con id = -1 e distanze = FLT_MAX
    for (int i = 0; i < k; i++) {
        KNN[i*2] = -1;
        KNN[i*2 + 1] = FLT_MAX;
    }

    // Calcola le distanze approssimate query -> pivot
    float* distanzaApprossimataQP = malloc(sizeof(float) * h);
    for (int i = 0; i < h; i++) {
        float *pivotCuscinetto = &pivot[i * D];
        distanzaApprossimataQP[i] = distanzaApprossimata(query, pivotCuscinetto);
    }

    // Scorri tutti i vettori del dataset
    for (int i = 0; i < N; i++) {
        float* vettore = &dataSet[i * D];
        float d_pvt_max = 0.0f;

        // Calcola lower-bound tramite pivot (disuguaglianza triangolare)
        for (int j = 0; j < h; j++) {
            float d_vi_pj = vettoreIndexing[i * h + j];  // distanza v->pivot (dall'indice)
            float d_q_pj  = distanzaApprossimataQP[j];   // distanza query->pivot
            float limiteInferiore = fabsf(d_vi_pj - d_q_pj);  // |d(v,p) - d(q,p)|
            if (limiteInferiore > d_pvt_max)
                d_pvt_max = limiteInferiore;
        }

        // Ottieni la distanza massima attuale nei K-NN
        float d_k_max = get_d_k_max(KNN, k);
        
        // PRIMO FILTRO: Se il limite inferiore è già troppo grande, scarta il punto
        if (d_pvt_max < d_k_max) {
            // SECONDO FILTRO: Calcola distanza approssimata per un filtro più preciso
            float d_q_v_approx = distanzaApprossimata(query, vettore);
            
            if (d_q_v_approx < d_k_max) {
                // TERZO FILTRO: Calcola la distanza REALE (euclidea)
                float d_q_v_reale = dEuclidea(query, vettore);
                
                // Inserisci solo se la distanza reale è minore
                if (d_q_v_reale < d_k_max) {
                    insert_into_knn(KNN, k, i, d_q_v_reale);
                }
            }
        }
    }

    // NON serve più ricalcolare le distanze: sono già euclidee!
    // (rimuovi il loop finale che avevi)

    free(distanzaApprossimataQP);
    return KNN;
}

//
// ---------------------------------------------------------------
//  FUNZIONI DI TEST
// ---------------------------------------------------------------
void testQuantizing(float *v, const char *name) {

    printf("\n\n===== TEST %s =====\n", name);

    float *vPlus  = calloc(D, sizeof(float));
    float *vMinus = calloc(D, sizeof(float));

    quantizing(v, vMinus, vPlus);

    printf("\nv: ");
    for (int i = 0; i < D; i++) printf("%.1f ", v[i]);

    printf("\nvPlus:  ");
    for (int i = 0; i < D; i++) printf("%.0f ", vPlus[i]);

    printf("\nvMinus: ");
    for (int i = 0; i < D; i++) printf("%.0f ", vMinus[i]);

    printf("\n");

    free(vPlus);
    free(vMinus);
}


int nomeFittizio() {

    float test1[] = {  2, -6, 1, -5, 3, 4 };
    testQuantizing(test1, "Valori misti");

    float test2[] = { 1, 9, 3, 7, 5 };
    testQuantizing(test2, "Tutti positivi");

    float test3[] = { -10, -1, -7, -3 };
    testQuantizing(test3, "Tutti negativi");

    float test4[] = { 5, -5, 5, -5 };
    testQuantizing(test4, "Valori uguali");

    float test5[] = { 1, 2, 3, 4 };
    testQuantizing(test5, "k > D");

    return 0;
}
//Genera due vettori casuali e calcola la distanza euclidea e quella approssimata. 
void testDistanzaApprossimata() {

    printf("\n===== TEST distanzaApprossimata =====\n");

    float v[D], w[D];

    for (int i = 0; i < D; i++) {
        v[i] = ((float)rand() / RAND_MAX) * 2 - 1;
        w[i] = ((float)rand() / RAND_MAX) * 2 - 1;
    }

    printf("v: ");
    for (int i = 0; i < D; i++) printf("%.3f ", v[i]);

    printf("\nw: ");
    for (int i = 0; i < D; i++) printf("%.3f ", w[i]);
    printf("\n");

    float reale = dEuclidea(v, w);
    float approx = distanzaApprossimata(v, w);

    printf("Distanza reale    = %.4f\n", reale);
    printf("Distanza approx   = %.4f\n", approx);
}
void testIndexing() {

    printf("\n===== TEST indexing =====\n");

    float *dataset = malloc(N * D * sizeof(float));
    float *pivots  = malloc(h * D * sizeof(float));

    // dataset casuale
    for (int i = 0; i < N; i++)
        for (int j = 0; j < D; j++)
            dataset[i*D+j] = ((float)rand()/RAND_MAX)* 20 - 10;;

    // pivots tramite tua funzione
    float *pv = calcoloPivot(dataset, h, N, D);
    memcpy(pivots, pv, h*D*sizeof(float));
    free(pv);

    printf("\nDataset:\n");
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < D; j++)
            printf("%.3f ", dataset[i*D+j]);
        printf("\n");
    }

    printf("\nPivots:\n");
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < D; j++)
            printf("%.3f ", pivots[i*D+j]);
        printf("\n");
    }

    // indexing usando la versione corretta della distanza
    float *IDX_app = malloc(N*h*sizeof(float));
    float *IDX_real = malloc(N*h*sizeof(float));

    printf("\nConfronto distanze reali vs approssimate (N x h):\n");
    for (int r = 0; r < N; r++) {
        for (int c = 0; c < h; c++) {
            float *vettore = &dataset[r*D];
            float *vPivot  = &pivots[c*D];

            IDX_real[r*h+c] = dEuclidea(vettore, vPivot);
            IDX_app[r*h+c]  = distanzaApprossimata(vettore, vPivot);

            printf("Riga %d Pivot %d: Reale = %.4f, Approssimata = %.4f\n",
                   r, c, IDX_real[r*h+c], IDX_app[r*h+c]);
        }
    }

    free(dataset);
    free(pivots);
    free(IDX_app);
    free(IDX_real);
}

void testQuerying() {
    printf("\n===== TEST querying =====\n");

    // Genera dataset casuale
    float *dataset = malloc(N * D * sizeof(float));
    for (int i = 0; i < N; i++)
        for (int j = 0; j < D; j++)
            dataset[i*D + j] = ((float)rand() / RAND_MAX) * 20 - 10;

    // Calcola i pivot
    float *pivot = calcoloPivot(dataset, h, N, D);

    // Costruisci indice approssimato
    float *vettoreIndexing = indexing(pivot, dataset);

    // Seleziona un vettore del dataset come query
    float *query = malloc(D * sizeof(float));
    for (int i = 0; i < D; i++)
        query[i] = ((float)rand() / RAND_MAX) * 20 - 10;

    printf("\nQuery casuale generata:\n");
    for (int i = 0; i < D; i++) printf("%.3f ", query[i]);
    printf("\n");

    // Esegui querying
    float *KNN = querying(query, pivot, dataset, vettoreIndexing);

    // Stampa risultati
    printf("\nK-NN trovati (id, distanza):\n");
    for (int i = 0; i < k; i++) {
        int id = (int)KNN[i * 2];
        float dist = KNN[i * 2 + 1];
        if (id != -1) {
            printf("%d: %.4f\n", id, dist);
        }
    }

    free(dataset);
    free(pivot);
    free(vettoreIndexing);
    free(KNN);
}

// Confronta i vicini trovati da querying con quelli reali calcolati a distanza euclidea
void testQueryingCompleto() {
    printf("\n===== TEST querying COMPLETO =====\n");

    // 1. Genera dataset casuale
    float *dataset = malloc(N * D * sizeof(float));
    for (int i = 0; i < N; i++)
        for (int j = 0; j < D; j++)
            dataset[i*D + j] = ((float)rand() / RAND_MAX) * 20 - 10;

    // 2. Calcola pivot e indice
    float *pivot = calcoloPivot(dataset, h, N, D);
    float *vettoreIndexing = indexing(pivot, dataset);

    // 3. Seleziona un vettore casuale come query
    float *query = malloc(D * sizeof(float));
    for (int i = 0; i < D; i++)
        query[i] = ((float)rand() / RAND_MAX) * 20 - 10;

    // 4. Esegui querying
    float *KNN = querying(query, pivot, dataset, vettoreIndexing);

    printf("\nK-NN trovati da querying (id, distanza approssimata -> reale):\n");
    for (int i = 0; i < k; i++) {
        int id = (int)KNN[i * 2];
        float dist = KNN[i * 2 + 1];
        if (id != -1) {
            printf("%d: %.4f\n", id, dist);
        }
    }

    // 5. Calcola K-NN reali (distanza euclidea)
    float *realDistances = malloc(N * sizeof(float));
    for (int i = 0; i < N; i++) {
        float *v = &dataset[i * D];
        realDistances[i] = dEuclidea(query, v);
    }

    // Ordina e stampa i K vicini reali
    printf("\nK-NN reali (distanza euclidea):\n");
    for (int n = 0; n < k; n++) {
        int min_idx = -1;
        float min_val = FLT_MAX;
        for (int i = 0; i < N; i++) {
            if (realDistances[i] < min_val) {
                min_val = realDistances[i];
                min_idx = i;
            }
        }
        if (min_idx != -1) {
            printf("%d: %.4f\n", min_idx, min_val);
            realDistances[min_idx] = FLT_MAX; // escludi già trovato
        }
    }
    printf("\n=== VERIFICA: punti esclusi ===\n");
    printf("Verifichiamo che tutti i punti esclusi siano più lontani...\n");
    
    float max_dist_knn = 0.0f;
    for (int i = 0; i < k; i++) {
        if (KNN[i*2+1] > max_dist_knn)
            max_dist_knn = KNN[i*2+1];
    }
    
    int errori = 0;
    for (int i = 0; i < N; i++) {
        // Controlla se questo punto è nei K-NN
        int nei_knn = 0;
        for (int j = 0; j < k; j++) {
            if ((int)KNN[j*2] == i) {
                nei_knn = 1;
                break;
            }
        }
        
        if (!nei_knn) {
            float *v = &dataset[i * D];
            float dist = dEuclidea(query, v);
            if (dist < max_dist_knn) {
                printf("ERRORE: punto %d escluso ma ha distanza %.4f < %.4f\n", 
                       i, dist, max_dist_knn);
                errori++;
            }
        }
    }
    
    if (errori == 0)
        printf("✓ Nessun errore! Tutti i punti esclusi sono effettivamente più lontani.\n");
    else
        printf("✗ Trovati %d errori!\n", errori);

    free(dataset);
    free(pivot);
    free(vettoreIndexing);
    free(KNN);
    free(realDistances);
}

//
// ---------------------------------------------------------------
//  MAIN
// ---------------------------------------------------------------
int main() {

    srand((unsigned)time(NULL));
    //printf(">>> VERSIONE CORRETTA COMPILATA <<<\n");

    //nomeFittizio();                
    //testDistanzaApprossimata();   
    //testIndexing();
    //testQuerying();              
    testQueryingCompleto();
    return 0;
}
