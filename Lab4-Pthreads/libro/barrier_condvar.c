// libro/barrier_condvar.c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

static int counter=0;
static int thread_count=0;
static pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_var=PTHREAD_COND_INITIALIZER;

void barrier_wait(){
    pthread_mutex_lock(&mutex);
    counter++;
    if(counter==thread_count){
        counter=0;
        pthread_cond_broadcast(&cond_var);
    }else{
        while(pthread_cond_wait(&cond_var,&mutex)!=0);
    }
    pthread_mutex_unlock(&mutex);
}

void* work(void* arg){
    long id=(long)arg;
    for(int phase=0;phase<200;++phase){
        barrier_wait();
    }
    return NULL;
}

int main(int argc,char** argv){
    thread_count=(argc>=2)?atoi(argv[1]):8;
    pthread_t* th=malloc(thread_count*sizeof(pthread_t));
    struct timeval t0,t1; gettimeofday(&t0,NULL);
    for(long t=0;t<thread_count;++t)pthread_create(&th[t],NULL,work,(void*)t);
    for(int t=0;t<thread_count;++t)pthread_join(th[t],NULL);
    gettimeofday(&t1,NULL);
    double ms=(t1.tv_sec-t0.tv_sec)*1000.0+(t1.tv_usec-t0.tv_usec)/1000.0;
    printf("barrier_condvar,%d,%d,%d,%.3f\n", thread_count, 65536, 200, ms);
    free(th);
    return 0;
}
