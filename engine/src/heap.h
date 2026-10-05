# ifndef HEAP_H
#define HEAP_H

#define HEAP_MAX 4096

typedef struct {
    int priority ;
    int value;
} HeapItem ;


typedef struct {
    HeapItem items[HEAP_MAX];
    int size ;

} Heap; 

void heap_init(Heap *h) ;
int heap_push(Heap *h ,int priority ,int value ) ;
int heap_pop (Heap *h , HeapItem * out) ;
int heap_is_empty(const Heap*h) ;


#endif 