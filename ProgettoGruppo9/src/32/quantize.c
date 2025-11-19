#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const float *arr_ctx;
/**
 * Legge da un file e restituisce un array di float 
 * INPUT: string s, contenente il path da leggere 
 * OUTPUT: puntatore ad un array di float di dimensione D 
 */

int comparatore(const void *a, const void *b) {
  int val1 = *(int*)a;
  int val2 = *(int*)b;
  return (abs(arr_ctx[val2]) - abs(arr_ctx[val1]));
}
/** 
 * INPUT: array v di dimensione D, parametro x che indica quanti massimi in v.assoluto cercare
 * OUTPUT: restituisce due array di interi v+ e v-, inizializzata con 0 e 1 nella posizione dei massimi trovati
 */
void quantizing(float *v, int D, int k, float *vMinus, float *vPlus) {
    // possibile FIXME: l'array potrebbe essere MOLTO grande, forse è meglio allocare
    // nell'heap (malloc) e non nello stack
    //int arr[] = { 2 ,6, 1, -5, 3, 4 };
    
    //int n = sizeof(arr) / sizeof(arr[0]);
    int array_indici[D];
    // 0, 1, 2, 3, 4, 5
    for(int i = 0; i < D; i++) 
      array_indici[i] = i;

    // mi aspetto di trovare 1, -3, 5, 4, 0, 2
    arr_ctx = v;
    qsort(array_indici, D, sizeof(int), comparatore);
    for (int i = 0; i < D; i++) {
      if (v[array_indici[i]] < 0)
        array_indici[i] = -array_indici[i];
    }

    for (int i = 0; i < D; i++) {
      printf("%d\t", array_indici[i]);
    }
    // ora prendo i primi X 
    // TODO: sostituire i numeri fittizzi con i parametri del metodo
    int x = 2; //numero fittizio per ora
    if(x > D) {
      fprintf(stderr, "ERRORE: hai richiesto %d massimi ma il dataset è grande al massimo %d", x, D);
    }

    for(int i = 0; i < x; i++) {
      // se negativo allora scrivo su v-
      int idx = array_indici[i];
      //TODO: sostituire il commento in cui vengono assegnati i valori con vettori esistenti in output
      if(idx < 0) {
        vMinus[-idx] = 1;
      } else {
        vPlus[idx] = 1;
      }
      // nel nostro esempio specifico mi aspetto di trovare la terza posizione 
      // di v_minus pari ad 1 e la prima e quinta posizione di v_plus pari ad 1
    }

    arr_ctx = NULL;
}
void run_test(float *v, int D, int x, const char *name) {
    printf("\n===== TEST %s =====\n", name);

    float *vPlus  = calloc(D, sizeof(float));
    float *vMinus = calloc(D, sizeof(float));

    quantizing(v, D, x, vMinus, vPlus);

    printf("\nv: ");
    for(int i=0;i<D;i++) printf("%.1f ", v[i]);

    printf("\nvPlus:  ");
    for(int i=0;i<D;i++) printf("%.0f ", vPlus[i]);

    printf("\nvMinus: ");
    for(int i=0;i<D;i++) printf("%.0f ", vMinus[i]);
    printf("\n");

    free(vPlus);
    free(vMinus);
}

int nomeFittizio() {

    float test1[] = { 2, -6, 1, -5, 3, 4 };
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
