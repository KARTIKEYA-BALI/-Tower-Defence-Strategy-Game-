#ifndef STACK_H
#define STACK_H 

typedef struct {
    int type; 
    int cell_idx;
    int tower_type; 
    int gold;
}StackItem;

typedef struct {
    StackItem * items; 
    int top; 
    int capacity ;
}Stack; 

int stack_init(Stack *s, int intial_capacity );
void stack_free(Stack *s) ;
int stack_push(Stack *s ,StackItem item) ; 
int stack_pop(Stack *s ,StackItem *out)  ; 
int stack_peek(const Stack *s ,StackItem * out); 
int stack_is_empty(const Stack *s) ;
void stack_clear(Stack *s) ;

#endif 