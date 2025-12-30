#include <pthread.h>
#include <atomic>
#include <vector>
#include <random>
#include <chrono>
#include <iostream>
#include <algorithm>

struct Task {
    const double* A;
    const double* x;
    double* y;
    int nrows, ncols, chunk;
    std::atomic<int> next_row{0};
};

// Hilo
void* worker(void* arg) {
    Task* t = (Task*)arg;
    while (true) {
        int start = t->next_row.fetch_add(t->chunk, std::memory_order_relaxed);
        if (start >= t->nrows) break;

        int end = std::min(start + t->chunk, t->nrows);
        for (int i = start; i < end; ++i) {
            const double* row = t->A + (size_t)i * t->ncols;
            double acc = 0.0;
            for (int j = 0; j < t->ncols; ++j)
                acc += row[j] * t->x[j];
            t->y[i] = acc;
        }
    }
    return nullptr;
}

int main(int argc, char** argv) {
    int n = 4000, m = 4000, T = 8, CH = 64;
    if (argc >= 5) { n = atoi(argv[1]); m = atoi(argv[2]); T = atoi(argv[3]); CH = atoi(argv[4]); }

    std::vector<double> A((size_t)n * m), x(m), y(n, 0.0);
    std::mt19937_64 rng(7);
    std::uniform_real_distribution<double> U(-1, 1);
    for (auto& v : A) v = U(rng);
    for (auto& v : x) v = U(rng);

    Task task{A.data(), x.data(), y.data(), n, m, CH};

    // Lanzar hilos
    auto t0 = std::chrono::high_resolution_clock::now();
    std::vector<pthread_t> threads(T);
    for (int t = 0; t < T; ++t)
        pthread_create(&threads[t], nullptr, worker, &task);
    for (int t = 0; t < T; ++t)
        pthread_join(threads[t], nullptr);
    auto t1 = std::chrono::high_resolution_clock::now();

    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    std::cout << "matvec_dynamic_simple," << n << "x" << m << "," << T << "," << CH << "," << ms << "\n";

    return 0;
}
