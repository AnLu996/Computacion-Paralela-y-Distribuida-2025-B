// propia/barrier_sense_ticket_simple.cpp
#include <pthread.h>
#include <atomic>
#include <vector>
#include <chrono>
#include <iostream>
#include <thread> 

struct TicketLock {
    std::atomic<unsigned> next{0};
    std::atomic<unsigned> serving{0};

    void lock() {
        unsigned me = next.fetch_add(1, std::memory_order_acq_rel);
        while (serving.load(std::memory_order_acquire) != me)
            std::this_thread::yield(); // cede CPU 
    }
    void unlock() {
        serving.fetch_add(1, std::memory_order_release);
    }
};

// Barrera
struct SenseBarrier {
    const int n;
    std::atomic<int> count;
    std::atomic<bool> sense;
    static thread_local bool local_sense;

    explicit SenseBarrier(int T) : n(T), count(T), sense(false) {}

    void wait() {
        local_sense = !local_sense;
        if (count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            count.store(n, std::memory_order_release);
            sense.store(local_sense, std::memory_order_release);
        } else {
            // Evita spin agresivo
            while (sense.load(std::memory_order_acquire) != local_sense)
                std::this_thread::yield();
        }
    }
};
thread_local bool SenseBarrier::local_sense = false;

struct Args {
    int id, T, N, rounds;
    SenseBarrier* B;
    TicketLock* L;
    std::vector<int>* buf;
};

void* worker(void* arg) {
    auto* a = (Args*)arg;
    int per = (a->N + a->T - 1) / a->T;
    int start = a->id * per;
    int end = std::min(start + per, a->N);

    for (int r = 0; r < a->rounds; ++r) {
        // Fase A: trabajo paralelo
        for (int i = start; i < end; ++i)
            (*a->buf)[i] = a->id + 1;

        a->B->wait();  // sincroniza fase A

        // Fase B: sección crítica breve
        a->L->lock();
        // trabajo crítico muy corto
        a->L->unlock();

        a->B->wait();  // sincroniza fase B
    }
    return nullptr;
}

int main(int argc, char** argv) {
    int T = (argc >= 2) ? std::atoi(argv[1]) : 8;
    int N = (argc >= 3) ? std::atoi(argv[2]) : (1 << 16);
    int R = (argc >= 4) ? std::atoi(argv[3]) : 50;

    SenseBarrier B(T);
    TicketLock L;
    std::vector<int> buffer(N, 0);
    std::vector<pthread_t> threads(T);
    std::vector<Args> args(T);

    auto t0 = std::chrono::steady_clock::now();

    for (int i = 0; i < T; ++i) {
        args[i] = {i, T, N, R, &B, &L, &buffer};
        pthread_create(&threads[i], nullptr, worker, &args[i]);
    }
    for (int i = 0; i < T; ++i)
        pthread_join(threads[i], nullptr);

    auto t1 = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    std::cout << "barrier_sense_ticket_simple,"
              << T << "," << N << "," << R << "," << ms << "\n";
    return 0;
}
