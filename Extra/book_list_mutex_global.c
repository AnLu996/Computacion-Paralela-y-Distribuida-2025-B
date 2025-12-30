// list_mutex_global.c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

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
        return 0; // duplicado
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

int main() {
    Insert(2); Insert(5); Insert(8);
    printf("Member(5)=%d\n", Member(5));
    Delete(5);
    printf("Member(5)=%d\n", Member(5));
    return 0;
}
