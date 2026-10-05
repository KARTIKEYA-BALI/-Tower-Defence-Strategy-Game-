#include "heap.h"

void heap_init(Heap *h) {
    h->size=0; 
}

static void swap (HeapItem * a ,HeapItem * b) {
    HeapItem temp =*a; 
    *a=*b; 
    *b=temp ; 
}

int heap_push(Heap * h, int priority ,int value ){
    if(h->size==HEAP_MAX){
        return 0; 
    }
    int i=h->size; 
    h->items[i].priority=priority; 
    h->items[i].value = value;
    h->size++;

    while(i>0){
        int parent = (i-1)/2 ;
        if(h->items[parent].priority <= h->items[i].priority ) break ;
        swap(&h->items[parent], &h->items[i]) ;
        i=parent ;
    }

    return 1 ;

}

int heap_pop( Heap * h, HeapItem * out ){
    if(h->size==0) return 0; 
    out->priority=h->items[0].priority; 
    out->value= h->items[0].value; 

    h->items[0]=h->items[h->size-1] ; 
    -- (h->size) ; 
    int i=0 ;

    while(1) {
        if(  (((i+1)*2-1)<h->size) && ( h->items[i].priority>h->items[(i+1)*2 -1 ].priority ) && (((i+1)*2 >= h->size) || (h->items[(i+1)*2 - 1].priority <= h->items[(i+1)*2].priority)) ){
            swap(&h->items[i], & h->items[(i+1)*2 -1]);
            i=(i+1)*2-1; 
        }
        else if((((i+1)*2)<h->size) && (h->items[i].priority>h->items[(i+1)*2 ].priority) ){
            swap(&h->items[i], & h->items[(i+1)*2 ]);
            i=(i+1)*2;
        }
        else{
            break;
        }
    }
    return 1; 

}

int heap_is_empty(const Heap*h) {
    return h->size==0 ;
}