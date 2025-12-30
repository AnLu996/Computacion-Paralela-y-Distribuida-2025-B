// barrier_condvar.c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

static int counter = 0;
static int thread_count = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_var = PTHREAD_COND_INITIALIZER;

void barrier_wait() {
    pthread_mutex_lock(&mutex);
    counter++;
    if (counter == thread_count) {
        counter = 0;                          // reutilizable
        pthread_cond_broadcast(&cond_var);    // despierta a todos
    } else {
        // IMPORTANTE: libera el mutex mientras espera
        while (pthread_cond_wait(&cond_var, &mutex) != 0);
    }
    pthread_mutex_unlock(&mutex);
}

void* work(void* arg){
    long id = (long)arg;
    // Fase A
    printf("T%ld: fase A\n", id);
    barrier_wait();
    // Fase B
    printf("T%ld: fase B\n", id);
    barrier_wait();
    return NULL;
}

int main(int argc, char** argv){
    thread_count = (argc>=2)? atoi(argv[1]) : 4;
    pthread_t* th = (pthread_t*)malloc(thread_count*sizeof(pthread_t));
    for (long t=0;t<thread_count;++t) pthread_create(&th[t], NULL, work, (void*)t);
    for (int t=0;t<thread_count;++t) pthread_join(th[t], NULL);
    return 0;
}
