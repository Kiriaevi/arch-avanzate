#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

  static int N = 5;
  static int D = 3;
  static int h = 2;

//
// ---------------------------------------------------------------
//  PROTOTIPI
// ---------------------------------------------------------------
float* readFile(char *f);
float* calcoloPivot(float *dataSet, int h, int N, int D);
float* executeQuery(float q);

float dEuclidea(float *v, float *w, const unsigned int D);
float prodScalare(float *v, float *w, const unsigned int D);

int comparatore(const void *a, const void *b);
void quantizing(float *v, int D, int k, float *vMinus, float *vPlus);

void run_test(float *v, int D, int x, const char *name);
int nomeFittizio();


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
    float* pivot = malloc(sizeof(float) * D * h);
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
float dEuclidea(float *v, float *w, const unsigned int D) {
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
float prodScalare(float *v, float *w, const unsigned int D) {
    float prod = 0.0f;
    for (int i = 0; i < D; i++) {
        prod += v[i] * w[i];
    }
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

    return (fabsf(arr_ctx[i2]) - fabsf(arr_ctx[i1]));
}

void quantizing(float *v, int D, int kUnused, float *vMinus, float *vPlus) {

    int array_indici[D];
    for (int i = 0; i < D; i++)
        array_indici[i] = i;

    arr_ctx = v;
    qsort(array_indici, D, sizeof(int), comparatore);

    // applico segno
    for (int i = 0; i < D; i++)
        if (v[array_indici[i]] < 0)
            array_indici[i] = -array_indici[i];

    printf("\nOrdine indici assoluti con segno: ");
    for (int i = 0; i < D; i++)
        printf("%d ", array_indici[i]);

    int x = 2;  // fittizio come nel tuo codice

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
}
float distanzaApprossimata(float *v, float *w){
  return 0.0f;
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
void run_test(float *v, int D, int x, const char *name) {

    printf("\n\n===== TEST %s =====\n", name);

    float *vPlus  = calloc(D, sizeof(float));
    float *vMinus = calloc(D, sizeof(float));

    quantizing(v, D, x, vMinus, vPlus);

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
    run_test(test1, 6, 2, "Valori misti");

    float test2[] = { 1, 9, 3, 7, 5 };
    run_test(test2, 5, 2, "Tutti positivi");

    float test3[] = { -10, -1, -7, -3 };
    run_test(test3, 4, 2, "Tutti negativi");

    float test4[] = { 5, -5, 5, -5 };
    run_test(test4, 4, 2, "Valori uguali");

    float test5[] = { 1, 2, 3, 4 };
    run_test(test5, 4, 6, "k > D");

    return 0;
}



//
// ---------------------------------------------------------------
//  MAIN
// ---------------------------------------------------------------
int main() {


    srand((unsigned int)time(NULL));

    float *array = malloc(sizeof(float) * N * D);

    // riempi dataset
    for (int i = 0; i < N; i++)
        for (int j = 0; j < D; j++)
            array[i*D + j] = (float)rand() / RAND_MAX;

    // stampa dataset
    printf("Dataset (%d x %d):\n", N, D);
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < D; j++)
            printf("%.3f ", array[i*D + j]);
        printf("\n");
    }

    // PIVOT
    float *pivots = calcoloPivot(array, h, N, D);

    printf("\nPivot:\n");
    for (int k = 0; k < h; k++) {
        for (int j = 0; j < D; j++)
            printf("%.3f ", pivots[k*D + j]);
        printf("\n");
    }

    // TEST quantize
    nomeFittizio();

    free(array);
    free(pivots);

    return 0;
}
