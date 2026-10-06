#include <stdlib.h>
# include "queue.h"

/* CIRCULAR QUEUE  */

int queue_init(Queue *q ,int capacity) {
    q->front=-1;
    q->rear=-1;
    if(capacity<=0) return 0 ;
    q->capacity=capacity ;
    q->items=(Event *) malloc(sizeof(Event)* (q->capacity)) ; 
    if(q->items==NULL) return 0;
    return 1; 

}

void queue_free(Queue *q) {
    free(q->items) ;
    q->items = NULL ;
    q->front=-1; 
    q->rear=-1; 
    q->capacity=0 ;
}

int queue_push(Queue *q, Event e){

    if((q->rear+1)%q->capacity==(q->front)) return 0;
    if(q->front == -1 ) {
        q->front =0 ;
        q->rear =0 ;
    }
    else{
        q->rear=(q->rear+1)%q->capacity;
    }
    q->items[q->rear]=e; ;
    return 1; 

}

int queue_pop(Queue *q ,Event * out){

    if(q->front == -1 ) return 0 ;

    if(q->front == q->rear) {
        * out =q->items[q->front];
        q->rear= -1; 
        q->front= -1; 
        return 1 ; 
    }

    * out =q->items[q->front ];
    q->front=(q->front+1) % (q->capacity);


    return 1 ;

}


int queue_peek(const Queue *q ,Event *out) {
   if(q->front == -1 ) return 0 ;
    * out =q->items[q->front];
    return 1 ;

}

int queue_is_empty(const Queue *q) {
    return q->front== -1  ;
}

int queue_is_full(const Queue *q){
    return  (q->rear +1)%(q->capacity)==(q->front) ;
}

void queue_clear (Queue *q) {
    q->front =-1 ;
    q->rear = -1; 
}