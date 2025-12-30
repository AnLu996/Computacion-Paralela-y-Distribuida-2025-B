// seguridad_hilos_barrier_ticket.cpp
#include <pthread.h>
#include <atomic>
#include <vector>
#include <iostream>
#include <chrono>

struct TicketLock {
    std::atomic<unsigned> next{0}, serving{0};
    void lock(){
        unsigned my = next.fetch_add(1, std::memory_order_acq_rel);
        while (serving.load(std::memory_order_acquire) != my) { /* spin */ }
    }
    void unlock(){
        serving.fetch_add(1, std::memory_order_release);
    }
};

struct SenseBarrier {
    const int n;
    std::atomic<int> count;
    std::atomic<bool> sense;
    thread_local static bool local_sense;
    SenseBarrier(int n_threads): n(n), count(n_threads), sense(false) {}
    void wait(){
        local_sense = !local_sense;
        if (count.fetch_sub(1, std::memory_order_acq_rel) == 1){
            count.store(n, std::memory_order_release);
            sense.store(local_sense, std::memory_order_release);
        } else {
            while (sense.load(std::memory_order_acquire) != local_sense) { /* spin */ }
        }
    }
};
thread_local bool SenseBarrier::local_sense = false;

struct Shared {
    TicketLock tlock;
    SenseBarrier barrier;
    std::vector<int> data;
    Shared(int T, int N): barrier(T), data(N, 0) {}
};

struct Args { Shared* S; int tid; int T; };

void* worker(void* a){
    auto* p = (Args*)a;
    // Fase 1: cada hilo escribe su “firma” en un segmento (sin lock)
    int n = (int)p->S->data.size();
    int per = (n + p->T - 1)/p->T;
    int beg = p->tid * per, end = std::min(beg+per, n);
    for(int i=beg;i<end;++i) p->S->data[i] = p->tid + 1;

    // Sincronizar fin de fase 1
    p->S->barrier.wait();

    // Fase 2: sección crítica con orden FIFO usando ticket lock
    p->S->tlock.lock();
    // simulamos trabajo corto en CS
    static int total_commits = 0;
    total_commits++;
    std::cout << "CS por hilo " << p->tid << " (commit #" << total_commits << ")\n";
    p->S->tlock.unlock();

    // Fase 3: otra sincronización antes de verificar
    p->S->barrier.wait();
    return nullptr;
}

int main(int argc, char** argv){
    int T=4, N=64;
    if(argc>=3){ T=std::atoi(argv[1]); N=std::atoi(argv[2]); }

    Shared S(T, N);
    std::vector<pthread_t> th(T);
    std::vector<Args> args(T);
    for(int t=0;t<T;++t){ args[t]={&S, t, T}; pthread_create(&th[t], nullptr, worker, &args[t]); }
    for(int t=0;t<T;++t) pthread_join(th[t], nullptr);

    // Verificación simple: todos los elementos escritos
    int ok = 1;
    for(int v: S.data) if(v==0) ok=0;
    std::cout << "Datos completados=" << ok << "\n";
    return 0;
}
