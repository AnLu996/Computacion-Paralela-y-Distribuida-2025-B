// bucle_for.h
#ifndef BUCLE_FOR_H
#define BUCLE_FOR_H

#include <iostream>
#include <vector>
#include <chrono>
using namespace std;
using hr = chrono::high_resolution_clock;

const int MAX = 512;
static double A[MAX][MAX], x[MAX], y[MAX];

void init_bucle_for() {
    for (int i = 0; i < MAX; i++) {
        x[i] = (i % 10) + 1;
        y[i] = 0;
        for (int j = 0; j < MAX; j++) {
            A[i][j] = (i + j) % 100 + 1;
        }
    }
}

void bucle_for_primer() {
    // y = 0
    for (int i = 0; i < MAX; i++) y[i] = 0;

    auto t0 = hr::now();
    for (int i = 0; i < MAX; i++) {
        for (int j = 0; j < MAX; j++) {
            y[i] += A[i][j] * x[j];
        }
    }
    auto t1 = hr::now();
    chrono::duration<double> dt = t1 - t0;
    cout << "Primer par de bucles (i-j): " << dt.count() << " s" << endl;
}

void bucle_for_segundo() {
    // y = 0
    for (int i = 0; i < MAX; i++) y[i] = 0;

    auto t0 = hr::now();
    for (int j = 0; j < MAX; j++) {
        for (int i = 0; i < MAX; i++) {
            y[i] += A[i][j] * x[j];
        }
    }
    auto t1 = hr::now();
    chrono::duration<double> dt = t1 - t0;
    cout << "Segundo par de bucles (j-i): " << dt.count() << " s" << endl;
}

#endif
