#include <stdio.h>
#include <stdlib.h>
#include "dynarray.h"

int intarray_init(IntArray * a ,int initial_capacity) {
    if(initial_capacity<=0) return 0 ;
    a->data=(int *) malloc(sizeof(int)*initial_capacity);
    a->capacity=initial_capacity;
    a->size=0;

    if(!(a->data)) return 0 ;
    return 1; 
}

int intarray_push(IntArray * a , int value){
    if(a->size==a->capacity) {
        int * new_point;
        a->capacity = (a->capacity==0) ? 4:a->capacity*2 ;
        new_point=(int*)realloc(a->data,sizeof(int)*a->capacity) ;
        if(new_point==NULL) return 0; 
        a->data=new_point; 
    }
    *(a->data+a->size)=value ;
    a->size++;
    return 1; 

}

int intarray_get(const IntArray *a ,int index, int *  out){
    if(index<0 || index>=a->size) return 0 ;

    *(out)=*(a->data+index);

    return 1; 
}

void intarray_clear(IntArray *a ){
    a->size=0;
}

void intarray_free(IntArray *a ) {
    a->capacity=0; 
    a->size=0 ;
    free(a->data);
    a->data=NULL ;
}