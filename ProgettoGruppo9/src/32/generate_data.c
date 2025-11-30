#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "common.h"

#define datasetFileName "generated_dataset.ds2"
#define queryFileName "generated_queries.ds2"
#define MAX_VALUE  20
#define MIN_VALUE  10

void save_data(const char* filename, void* arr, int N, int D)
{
    FILE* fp = fopen(filename, "wb");
    // i primi 8 byte vengono dedicati alla dimensione
    fwrite(&N, sizeof(int), 1, fp);
    fwrite(&D, sizeof(int), 1, fp);


    char* ptr = (char*) arr;
    for (int i = 0; i < N; i++) {
      fwrite(ptr, sizeof(type), D, fp);
      ptr += sizeof(type) * D;
    }
    fclose(fp);
}
int main (int argc, char **argv) {

  srand(time(NULL));
  if (argc < 3) {
    fprintf(stderr, "Errore, devi passarmi N e D");
    return(-2);
  }

  int N = atoi(argv[1]);
  int D = atoi(argv[2]);

  VECTOR dataset = (VECTOR) malloc(N * D *  sizeof(type));
  if (!dataset)
  {
    fprintf(stderr, "Errore allocazione dataset!\n");
    return -1;
  }
  for (int i = 0; i < N; i++)
    for (int j = 0; j < D; j++)
      dataset[i * D + j] = ((type)rand() / RAND_MAX) * MAX_VALUE - MIN_VALUE;
  // scrivi su file
  save_data(datasetFileName, dataset, N, D);
  free(dataset);

  VECTOR query = (VECTOR)malloc(N * D * sizeof(type));
  if (!query)
  {
    fprintf(stderr, "Errore allocazione query!\n");
    free(query);
    return -1;
  }
  for (int i = 0; i < D; i++)
    query[i] = ((type)rand() / RAND_MAX) * MAX_VALUE - MIN_VALUE;
  // scrivi su file
  save_data(queryFileName, query, N, D);
  free(query);

  return 0;
}
