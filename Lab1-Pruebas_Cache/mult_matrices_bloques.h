// mult_matrices_bloques.h
#ifndef MULT_MATRICES_BLOQUES_H
#define MULT_MATRICES_BLOQUES_H

#include <iostream>
#include <vector>
#include <chrono>
using namespace std;
using hr = chrono::high_resolution_clock;

void mult_matrices_bloques(int N, int B) {
    vector<double> A(N * N), Bm(N * N), C(N * N, 0.0);
    for (int i = 0; i < N * N; i++) {
        A[i] = (i % 100) + 1;
        Bm[i] = ((i + 1) % 100) + 1;
    }

    auto t0 = hr::now();
    for (int ii = 0; ii < N; ii += B) {
        for (int jj = 0; jj < N; jj += B) {
            for (int kk = 0; kk < N; kk += B) {
                int iimax = min(ii + B, N);
                int jjmax = min(jj + B, N);
                int kkmax = min(kk + B, N);
                for (int i = ii; i < iimax; i++) {
                    for (int k = kk; k < kkmax; k++) {
                        double a = A[i * N + k];
                        for (int j = jj; j < jjmax; j++) {
                            C[i * N + j] += a * Bm[k * N + j];
                        }
                    }
                }
            }
        }
    }
    auto t1 = hr::now();
    chrono::duration<double> dt = t1 - t0;
    cout << "Multiplicacion por bloques (" << N << ", bloque " << B << "): "
         << dt.count() << " s" << endl;

    volatile double sink = C[0]; (void)sink;
}

#endif
