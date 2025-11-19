/**
 * Calcola la distanza euclidea tra un punto referenziato da id e una distanza d
 * INPUT: int id, id dell'elemento generico a cui è necessario accedere con
 * v[i], float d, distanza OUTPUT: float out, distanza calcolata
 */
#include <math.h>
float dEuclidea(float *v, float *w, const unsigned int D) {
  float distanza = 0.0f;
  for (int i = 0; i < D; i++) {
    float parDif = v[i] - w[i];
    float sq = parDif * parDif;
    distanza += sq;
  }
  return sqrt(distanza);
}

/**
 * Dati due vettori v e w ne effettua il prodotto scalare
 * INPUT: array di float v, array di float w
 * OUTPUT: un float che restituisce la distanza
 */
float prodScalare(float *v, float *w, const unsigned int D) {
  float prod = 0.0f;
  for (int i = 0; i < D; i++) {
    prod += v[i] * w[i];
  }
  return prod;
}
