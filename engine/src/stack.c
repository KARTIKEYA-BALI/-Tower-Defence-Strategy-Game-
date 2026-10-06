#include <stdio.h>
#include <stdlib.h>
#include "stack.h"

int stack_init(Stack *s, int initial_capacity ){
    if(initial_capacity<=0) return 0 ; 
    s->capacity=initial_capacity ;
    s->top=-1; // as we are treating size as our top only 
    s->items=(StackItem *) malloc(sizeof(StackItem)*initial_capacity);
    if(s->items ==NULL) return 0;
    return 1; 

}

void stack_free(Stack *s) {
    free(s->items) ;
    s->items=NULL ;
    s->top=-1;
    s->capacity=0;  
}


int stack_push(Stack *s ,StackItem item) {
    if(s->top == s->capacity-1) {
        StackItem * new_point ;
        s->capacity=(s->capacity==0) ? 4:s->capacity*2;
        new_point =(StackItem *) realloc(s->items,sizeof(StackItem)*s->capacity);
        
        if(new_point==NULL) return 0; 
        s->items=new_point; 
        
    }
    ++s->top;
    *(s->items+s->top)=item;

    return 1; 

}



int stack_pop(Stack *s ,StackItem *out)  {
    if(s->top ==-1) return 0 ;
    *out = (s->items[(s->top)--]) ;
    return 1;
}


int stack_peek(const Stack *s ,StackItem * out){
    if(s->top ==-1) return 0 ;
    *out = (s->items[(s->top)]) ;
    return 1;
}


int stack_is_empty(const Stack *s) {
    return s->top==-1; 
}

void stack_clear(Stack *s) {
    s->top=-1; 
    
}
