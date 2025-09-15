// mult_matrices.h
#ifndef MULT_MATRICES_H
#define MULT_MATRICES_H

#include <iostream>
#include <vector>
#include <chrono>
using namespace std;
using hr = chrono::high_resolution_clock;

void mult_matrices_clasica(int N) {
    vector<double> A(N * N), B(N * N), C(N * N, 0.0);
    for (int i = 0; i < N * N; i++) {
        A[i] = (i % 100) + 1;
        B[i] = ((i + 1) % 100) + 1;
    }

    auto t0 = hr::now();
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            double s = 0.0;
            for (int k = 0; k < N; k++) {
                s += A[i * N + k] * B[k * N + j];
            }
            C[i * N + j] = s;
        }
    }
    auto t1 = hr::now();
    chrono::duration<double> dt = t1 - t0;
    cout << "Multiplicacion clasica (" << N << "): " << dt.count() << " s" << endl;

    volatile double sink = C[0]; (void)sink;
}

#endif
