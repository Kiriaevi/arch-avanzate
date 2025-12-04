#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define LINE_LEN 256

#define idsProf "results_ids_2000x8_k8_x64_32.ds2"
#define ids "results_ids_2000x8_k8_x64_32.ds2"
#define dstProf "results_ids_2000x8_k8_x64_32.ds2"
#define dst "results_ids_2000x8_k8_x64_32.ds2"

// Calcola errore relativo: |a - b| / max(|b|, eps)
double relative_error(double out, double ref) {
    const double eps = 1e-12;
    double denom = fabs(ref) > eps ? fabs(ref) : eps;
    return fabs(out - ref) / denom;
}

// Legge i valori da un file .ds2
int read_values(const char *filename, double **values, int *count) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Errore apertura file: %s\n", filename);
        return 0;
    }

    char line[LINE_LEN];
    int capacity = 1024;
    int n = 0;
    double *arr = malloc(capacity * sizeof(double));

    while (fgets(line, LINE_LEN, fp)) {
        if (n >= capacity) {
            capacity *= 2;
            arr = realloc(arr, capacity * sizeof(double));
        }
        arr[n++] = atof(line);
    }

    fclose(fp);
    *values = arr;
    *count = n;
    return 1;
}

int main() {
    double *out_idnn, *ref_ids;
    double *out_distnn, *ref_dist;
    int n_id, n_ref_id, n_dist, n_ref_dist;

    // Caricamento file
    if (!read_values(dst, &out_idnn, &n_id)) return 1;
    if (!read_values(dstProf, &ref_ids, &n_ref_id)) return 1;

    if (!read_values(ids, &out_distnn, &n_dist)) return 1;
    if (!read_values(idsProf, &ref_dist, &n_ref_dist)) return 1;

    if (n_id != n_ref_id || n_dist != n_ref_dist) {
        fprintf(stderr, "Errore: i file non hanno lo stesso numero di righe!\n");
        return 1;
    }

    double err_id_total = 0.0;
    double err_dist_total = 0.0;

    // --- Confronto ID esatti ---
    for (int i = 0; i < n_id; i++) {
        if (out_idnn[i] != ref_ids[i]) {
            err_id_total += relative_error(out_idnn[i], ref_ids[i]);
        }
    }

    // --- Confronto DIST con primi 3 decimali ---
    for (int i = 0; i < n_dist; i++) {
        double out_r = round(out_distnn[i] * 1000.0) / 1000.0;
        double ref_r = round(ref_dist[i]   * 1000.0) / 1000.0;

        if (fabs(out_r - ref_r) > 1e-9) {
            err_dist_total += relative_error(out_distnn[i], ref_dist[i]);
        }
    }

    // --- Risultati ---
    printf("=== RISULTATI ===\n");
    printf("Errore relativo totale ID     : %.12f\n", err_id_total);
    printf("Errore relativo totale DIST   : %.12f\n", err_dist_total);

    // Libera memoria
    free(out_idnn);
    free(ref_ids);
    free(out_distnn);
    free(ref_dist);

    return 0;
}

