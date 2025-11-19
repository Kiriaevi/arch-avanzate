#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const int *arr_ctx;
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
void quantizing() {
    // possibile FIXME: l'array potrebbe essere MOLTO grande, forse è meglio allocare
    // nell'heap (malloc) e non nello stack
    int arr[] = { 2 ,6, 1, -5, 3, 4 };
    
    int n = sizeof(arr) / sizeof(arr[0]);

    int array_indici[n];
    // 0, 1, 2, 3, 4, 5
    for(int i = 0; i < n; i++) 
      array_indici[i] = i;

    // mi aspetto di trovare 1, -3, 5, 4, 0, 2
    arr_ctx = arr;
    qsort(array_indici, n, sizeof(int), comparatore);
    for (int i = 0; i < n; i++) {
      if (arr[array_indici[i]] < 0)
        array_indici[i] = -array_indici[i];
    }

    for (int i = 0; i < n; i++) {
      printf("%d\t", array_indici[i]);
    }
    // ora prendo i primi X 
    // TODO: sostituire i numeri fittizzi con i parametri del metodo
    int D = 5; //numero fittizio per ora
    int x = 2; //numero fittizio per ora
    if(x > D) {
      fprintf(stderr, "ERRORE: hai richiesto %d massimi ma il dataset è grande al massimo %d", x, D);
    }

    for(int i = 0; i < x; i++) {
      // se negativo allora scrivo su v-
      int idx = array_indici[i];
      //TODO: sostituire il commento in cui vengono assegnati i valori con vettori esistenti in output
      if(idx < 0) {
        //v_minus[-idx] = 1
      } else {
        //v_plus[idx] = 1
      }
      // nel nostro esempio specifico mi aspetto di trovare la terza posizione 
      // di v_minus pari ad 1 e la prima e quinta posizione di v_plus pari ad 1
    }

    arr_ctx = NULL;
}