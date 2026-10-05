#ifndef DYNA_H
#define DYNA_H

typedef struct {
    int * data; 
    int size; 
    int capacity ;

}IntArray;

int intarray_init(IntArray * a ,int initial_capacity) ;
int intarray_push(IntArray * a , int value); 
int intarray_get(const IntArray *a ,int index, int *  out);
void intarray_clear(IntArray *a ); 
void intarray_free(IntArray *a ) ;




#endif 