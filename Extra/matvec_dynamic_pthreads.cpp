// matvec_dynamic_pthreads.cpp
#include <pthread.h>
#include <atomic>
#include <vector>
#include <random>
#include <iostream>

struct Task {
    const double* A;   // matriz aplanada por filas
    const double* x;   // vector
    double* y;         // salida
    int nrows, ncols;
    int chunk;         // tamaño de paquete de filas
    std::atomic<int> next_row{0};
};

void* worker(void* arg){
    Task* t = (Task*)arg;
    while (true){
        int start = t->next_row.fetch_add(t->chunk, std::memory_order_relaxed);
        if (start >= t->nrows) break;
        int end = std::min(start + t->chunk, t->nrows);
        for (int i = start; i < end; ++i){
            double acc = 0.0;
            const double* row = t->A + (size_t)i * t->ncols;
            for (int j = 0; j < t->ncols; ++j) acc += row[j] * t->x[j];
            t->y[i] = acc;
        }
    }
    return nullptr;
}

int main(int argc, char** argv){
    int n=2000, m=2000, T=8, CH=32;
    if(argc>=5){ n=std::atoi(argv[1]); m=std::atoi(argv[2]); T=std::atoi(argv[3]); CH=std::atoi(argv[4]); }

    std::vector<double> A((size_t)n*m), x(m), y(n,0.0);

    std::mt19937_64 rng(123);
    std::uniform_real_distribution<double> U(-1,1);
    for(auto& v:A) v=U(rng);
    for(auto& v:x) v=U(rng);

    Task task{A.data(), x.data(), y.data(), n, m, CH};
    std::vector<pthread_t> th(T);
    for(int t=0;t<T;++t) pthread_create(&th[t], nullptr, worker, &task);
    for(int t=0;t<T;++t) pthread_join(th[t], nullptr);

    // verificación simple
    std::cout << "y[0]=" << y[0] << ", y[n-1]=" << y[n-1] << "\n";
    return 0;
}
