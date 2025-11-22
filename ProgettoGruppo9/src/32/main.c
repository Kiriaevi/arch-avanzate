#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

static int N = 3; //Righe dataset
static int D = 10; //Colonne dataset
static int h = 1; // numero di pivot
static int x = 3; // parametro di quantizzazione
static int k = 3; // numero di vicini

#define FLT_MAX 3.402823466e+38F

//
// ---------------------------------------------------------------
//  PROTOTIPI
// ---------------------------------------------------------------
float* readFile(char *f);
float* calcoloPivot(float *dataSet, int h, int N, int D);
float* executeQuery(float q);

float dEuclidea(float *v, float *w);
float prodScalare(float *v, float *w);

int comparatore(const void *a, const void *b);
void quantizing(float *v, float *vMinus, float *vPlus);

void testQuantizing(float *v, const char *name);
int nomeFittizio();

float* querying(float *query, float *pivot, float *dataSet);

//
// ---------------------------------------------------------------
//  QUERYING
// ---------------------------------------------------------------
float* querying(float *query, float *pivot, float *dataSet){
    // Il vettore KNN è lungo K elementi, ognuno a D dimensioni.
    int dimensioneKnn = 2*k;
    float* KNN = malloc(sizeof(float) * dimensioneKnn);
    int j = 0;
    // Inizializza i KNN di q a distanza +inf
    for (int i = 0; i < dimensioneKnn; i++)
    {
        if (j % 2 == 0)
        {
            KNN[i] = -1;
        }
        else{
            KNN[i] = FLT_MAX;
        }
        j = (j+1)%2;
        
    }
    float* distanzaApprossimataQP = malloc(sizeof(float) * h); // Questo è lungo tanto quanto una riga del dataset
    //Itero per ricostruire i pivot
    int index = 0;
    float* pivotCuscinetto = calloc(D, sizeof(float)); // alloco un vettore temporaneo tutto nullo, lo aggiorno ad ogni iterazione dei pivot
    for (int i = 0; i < D * h; i+= D)
    {
        //Il ciclo esterno seleziona il pivot i esimo
        for (int j = 0; j < D; j++)
        {
            // il ciclo interno, invece, seleziona elemento per elemento del pivot i esimo
            pivotCuscinetto[j] = pivot[i + j];
        }
        distanzaApprossimataQP[index] = distanzaApprossimata(query,pivotCuscinetto);
        index++;
    }
    // Precalcoliamo la distanza approssimata (vettoreDataset - pivot). È grande N elementi, uno per riga
    float* distanzaApprossimataVP = malloc(sizeof(float) * N); 
    // Creo un vettore cuscinetto per estrarre la riga dal dataset
    float* vettoreCuscinetto = malloc(sizeof(float) * D);
    for (int i = 0; i < D * N; i+=D)
    {
        for (int j = 0; j < D; j++)
        {
            vettoreCuscinetto[j] = dataSet[i+j];
        }
        // Ho estratto il vettore, ora calcolo la distanza approssimata da ogni pivot
        // 
        for (int indexPivot = 0; indexPivot < D * h; indexPivot+=D)
        {
            for (int elementoPivot = 0; elementoPivot < D; elementoPivot++)
            {
                
            }
            
        }
        
        
    }
    free(vettoreCuscinetto);
    free(distanzaApprossimataVP);
    free(pivotCuscinetto);
    free(distanzaApprossimataQP);
    free(KNN);
}


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
    printf("Calcolo prodotto scalare fra \n");
    for (int i = 0; i < D; i++){
        printf("%.2f ",v[i]);
    }
    printf("\n");
    for (int i = 0; i < D; i++){
        printf("%.2f ",w[i]);
    }
    printf("\n");
    float prod = 0.0f;
    for (int i = 0; i < D; i++) {
        prod += v[i] * w[i];
    }
    printf("Risultato = %.5f \n",prod);
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
    printf("\nQuantizing di: ");
        for (int i = 0; i < D; i++) 
            printf("%.2f ", v[i]);
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

    printf("\nOrdine indici assoluti con segno: ");
    for (int i = 0; i < D; i++)
        printf("%d ", array_indici[i]);

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
}
float distanzaApprossimata(float *v, float *w) {
    // Alloco i vettori quantizzati
    float *vMinus = calloc(D, sizeof(float));
    float *vPlus  = calloc(D, sizeof(float));
    float *wMinus = calloc(D, sizeof(float));
    float *wPlus  = calloc(D, sizeof(float));
    // Quantizzo v e w
    printf("\n Quantize del primo vettore\n");
    quantizing(v, vMinus, vPlus);
    printf("\nQuantize del secondo vettore\n");
    quantizing(w, wMinus, wPlus);
    // Calcolo i prodotti scalari
    printf("Calcolo dei prodotti scalari dei due vettori\n");
    float posPos = prodScalare(vPlus, wPlus);
    float negNeg = prodScalare(vMinus, wMinus);
    float posNeg = prodScalare(vPlus, wMinus);
    float negPos = prodScalare(vMinus, wPlus);
    // Stampo tutti i termini
    printf("\nDistanza approssimata:\n");
    printf("v+ · w+ = %.5f\n", posPos);
    printf("v- · w- = %.5f\n", negNeg);
    printf("v+ · w- = %.5f\n", posNeg);
    printf("v- · w+ = %.5f\n", negPos);
    float approx = posPos + negNeg - posNeg - negPos;
    printf("Distanza approssimata = %.5f\n", approx);
    // Libero la memoria
    free(vMinus);
    free(vPlus);
    free(wMinus);
    free(wPlus);
    return approx;
}

float* creaVettore(float *v, int idx){
    float* output = malloc(D*sizeof(float));
    for(int j = 0; j < D; j++) {
        output[j] = v[idx*D + j]; 
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


//
// ---------------------------------------------------------------
//  MAIN
// ---------------------------------------------------------------
int main() {

    srand((unsigned)time(NULL));
    printf(">>> VERSIONE CORRETTA COMPILATA <<<\n");

    //nomeFittizio();                
    //testDistanzaApprossimata();   
    testIndexing();              

    return 0;
}
