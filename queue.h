#ifndef _QUEUE_H_
#define _QUEUE_H_

struct nodeObj {
    struct nodeObj* next;
    int *connfd;
    int *cachefl; // flag for cache
 };
typedef struct nodeObj node;

void enqueue(int *, int*);
int *dequeue();
int *logCheck();

#endif