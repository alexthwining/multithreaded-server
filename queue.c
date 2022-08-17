#include <stdlib.h>
#include "queue.h"

node* head = NULL;
node* tail = NULL;

// adds node to queue
void enqueue(int *data, int* cachefl) {
    node *tempNode = malloc(sizeof(node));
    tempNode->connfd = data;
    tempNode->cachefl = cachefl;
    tempNode->next = NULL;
    if(tail == NULL) {
        head = tempNode;
    }
    else {
        tail->next = tempNode;
    }

    tail = tempNode;
}

// return -1 if queue is empty
// return pointer to connfd
int *dequeue() {
    if(head == NULL) {
        return NULL;
    }
    else {
        int *res = head->connfd;
        node *temp = head;
        head = head->next;
        if(head == NULL) {
            tail = NULL;
        }
        free(temp);
        return res;
    }
}

// returns logging flag
int *cacheCheck() { 
    if(head == NULL) {
        return NULL;
    }
    else {
        return head->cachefl;
    }
}