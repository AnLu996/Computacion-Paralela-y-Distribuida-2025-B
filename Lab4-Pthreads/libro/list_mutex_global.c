// libro/list_mutex_global.c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

typedef struct node {
    int data;
    struct node* next;
} node;

static node* head_p = NULL;
static pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;

int Member(int value) {
    pthread_mutex_lock(&list_mutex);
    node* curr = head_p;
    while (curr != NULL && curr->data < value) curr = curr->next;
    int found = (curr != NULL && curr->data == value);
    pthread_mutex_unlock(&list_mutex);
    return found;
}

int Insert(int value) {
    pthread_mutex_lock(&list_mutex);
    node* curr = head_p;
    node* pred = NULL;
    while (curr != NULL && curr->data < value) { pred = curr; curr = curr->next; }
    if (curr == NULL || curr->data > value) {
        node* tmp = (node*)malloc(sizeof(node));
        tmp->data = value; tmp->next = curr;
        if (pred == NULL) head_p = tmp; else pred->next = tmp;
        pthread_mutex_unlock(&list_mutex);
        return 1;
    } else {
        pthread_mutex_unlock(&list_mutex);
        return 0;
    }
}

int Delete(int value) {
    pthread_mutex_lock(&list_mutex);
    node* curr = head_p;
    node* pred = NULL;
    while (curr != NULL && curr->data < value) { pred = curr; curr = curr->next; }
    if (curr != NULL && curr->data == value) {
        if (pred == NULL) head_p = curr->next; else pred->next = curr->next;
        free(curr);
        pthread_mutex_unlock(&list_mutex);
        return 1;
    } else {
        pthread_mutex_unlock(&list_mutex);
        return 0;
    }
}

void* thread_func(void* arg){
    long ops = ((long*)arg)[0];
    for (long i=0;i<ops;++i){
        int val = rand()%2000;
        int r = rand()%100;
        if (r<50) Member(val);
        else if (r<75) Insert(val);
        else Delete(val);
    }
    return NULL;
}

int main(int argc, char** argv){
    long n_ops = (argc>=2)? atol(argv[1]):200000;
    int T = (argc>=3)? atoi(argv[2]):8;

    for (int i=1;i<=1000;i++) Insert(i*2);

    pthread_t* th = malloc(T*sizeof(pthread_t));
    long arg[1]; arg[0]=n_ops/T;

    struct timeval t0,t1; gettimeofday(&t0,NULL);
    for (int t=0;t<T;++t) pthread_create(&th[t],NULL,thread_func,arg);
    for (int t=0;t<T;++t) pthread_join(th[t],NULL);
    gettimeofday(&t1,NULL);
    double ms=(t1.tv_sec-t0.tv_sec)*1000.0+(t1.tv_usec-t0.tv_usec)/1000.0;
    printf("list_mutex_global,%ld,%d,%.3f\n",n_ops,T,ms);
    free(th);
    return 0;
}
