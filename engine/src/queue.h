#ifndef QUEUE_H 
#define QUEUE_H

typedef struct { 
    int type; 
    int entity_id;
    int cell ;
    int value ;
}Event; 


typedef struct {
    Event* items; 
    int front ; 
    int rear ;
    int capacity ;

}Queue; 

int queue_init(Queue *q ,int capacity) ;
void queue_free(Queue *q) ;
int queue_push(Queue *q, Event e);
int queue_pop(Queue *q ,Event * out);
int queue_peek(const Queue *q ,Event *out) ;
int queue_is_empty(const Queue *q) ;
int queue_is_full(const Queue *q);
void queue_clear (Queue *q) ;

#endif 
