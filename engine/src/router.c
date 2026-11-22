#include <stdlib.h> 
#include "router.h"
#include "grid.h"
#include "queue.h"
#include "heap.h"



int route_path_exists(const Grid *g,int start ,int goal){ 
    int num_neigh,nx,ny,found=0;
    char*vis=0;;
    int end_cell=g->width*g->height,neighbours[4];

    if(!((start>=0 && start<end_cell) && (goal>=0 && goal<end_cell))) return 0;
    nx=start%g->width;
    ny=start/g->width;

    if(!(grid_is_walkable(g,nx,ny))) return 0;
    nx=goal%g->width;
    ny=goal/g->width;

    if(!(grid_is_walkable(g,nx,ny))) return 0;
    if(start==goal) return 1; 
    vis=(char *)calloc(end_cell,sizeof(char));

    if(vis==0) return 0;
    vis[start]=1;

    Queue q;
    Event e={0};
    e.cell=start; 
    if(!queue_init(&q,end_cell)){free(vis);return 0;}
    queue_push(&q,e); 

    while(!queue_is_empty(&q)){    
        queue_pop(&q,&e);
        int c_idx=e.cell;
        if(c_idx==goal){found=1; break ;}
        num_neigh=grid_neighbours(g,c_idx,neighbours); 

        while(num_neigh){
            nx=neighbours[num_neigh-1]%g->width;
            ny=neighbours[num_neigh-1]/g->width;
            if(grid_is_walkable(g,nx,ny) && (!(vis[neighbours[num_neigh-1]])) ){
                vis[neighbours[num_neigh-1]]=1;
                e.cell=neighbours[num_neigh-1] ;
                queue_push(&q,e);
            }
            num_neigh-- ;
        }

    }
    queue_free(&q);
    free(vis) ; 
    return found ;  
}

int route_can_place(Grid *g,int x , int y ,int start ,int goal){
    if(!(grid_in_bounds(g,x,y))) return 0; 
    int idx=y*g->width+x; 
    
    if(g->cells[idx].terrain!=CELL_BUILDABLE || g->cells[idx].tower_id!=-1 ) return 0;

    g->cells[idx].tower_id=69;

    int ok=(route_path_exists(g,start,goal)) ;
    g->cells[idx].tower_id=-1;

    return ok ;

}


static int step_cost(const Grid *g, int cell){
    (void)g;
    (void)cell;
    return 1;
}

int route_build_distance_field(const Grid *g, int goal, int dist[]){
    static Heap h;
    HeapItem item;
    int end_cell=g->width*g->height,neighbours[4],num_neigh,nx,ny;

    for(int i=0;i<end_cell;i++) dist[i]=ROUTE_UNREACHABLE;
    
    if(goal<0 || goal>=end_cell) return 0;
    if(!grid_is_walkable(g,goal%g->width,goal/g->width)) return 0;

    heap_init(&h);
    dist[goal]=0;
    heap_push(&h,0,goal);
    while(heap_pop(&h,&item)){
        int c=item.value;
        int d=item.priority;

        if(d>dist[c]) continue;
        num_neigh=grid_neighbours(g,c,neighbours);
        
        for(int k=0;k<num_neigh;k++){
            int nb=neighbours[k];
            nx=nb%g->width;
            ny=nb/g->width;
        
            if(!grid_is_walkable(g,nx,ny)) continue;
            int nd=d+step_cost(g,nb);
            if(dist[nb]==ROUTE_UNREACHABLE || nd<dist[nb]){
                dist[nb]=nd;
                if(!heap_push(&h,nd,nb)) return 0;
            } 
        }
    }
    return 1;
}

int route_next_step(const Grid *g, const int dist[], int cell){
    int end_cell=g->width*g->height,neighbours[4],num_neigh;
    if(cell<0 || cell>=end_cell) return -1;
    if(dist[cell]==ROUTE_UNREACHABLE || dist[cell]==0) return cell;
    int best=cell,best_d=dist[cell];
    num_neigh=grid_neighbours(g,cell,neighbours);
    for(int k=0;k<num_neigh;k++){
        int nb=neighbours[k];
        if(dist[nb]!=ROUTE_UNREACHABLE && dist[nb]<best_d){
            best=nb;
            best_d=dist[nb];
        }
    }
    return best;
}

static int drone_can_fly(const Grid *g, int cell){
    return g->cells[cell].terrain!=CELL_BLOCKED;
}

static int drone_step_cost(const Grid *g, int cell){
    return 1+DRONE_DANGER_WEIGHT*g->cells[cell].danger;
}

int route_drone_path(const Grid *g, int start, int goal, int out[], int max_len){
    static Heap h;
    HeapItem item;
    int end_cell=g->width*g->height,neighbours[4],num_neigh,len=0,c;
    int *dist,*prev;

    if(start<0 || start>=end_cell || goal<0 || goal>=end_cell || max_len<1) return 0;
    if(!drone_can_fly(g,start) || !drone_can_fly(g,goal)) return 0;

    dist=(int *)malloc(end_cell*sizeof(int));
    prev=(int *)malloc(end_cell*sizeof(int));
    if(dist==NULL || prev==NULL){
        free(dist);
        free(prev);
        return 0;
    }
    for(int i=0;i<end_cell;i++){
        dist[i]=ROUTE_UNREACHABLE;
        prev[i]=-1;
    }

    heap_init(&h);
    dist[start]=0;
    heap_push(&h,0,start);
    while(heap_pop(&h,&item)){
        c=item.value;
        if(item.priority>dist[c]) continue;
        if(c==goal) break;
        num_neigh=grid_neighbours(g,c,neighbours);
        for(int k=0;k<num_neigh;k++){
            int nb=neighbours[k];
            if(!drone_can_fly(g,nb)) continue;
            int nd=item.priority+drone_step_cost(g,nb);
            if(dist[nb]==ROUTE_UNREACHABLE || nd<dist[nb]){
                dist[nb]=nd;
                prev[nb]=c;
                if(!heap_push(&h,nd,nb)){
                    free(dist);
                    free(prev);
                    return 0;
                }
            }
        }
    }

    if(dist[goal]!=ROUTE_UNREACHABLE){
        for(c=goal;c!=-1;c=prev[c]) len++;
        if(len<=max_len){
            c=goal;
            for(int i=len-1;i>=0;i--){
                out[i]=c;
                c=prev[c];
            }
        }
        else len=0;
    }
    free(dist);
    free(prev);
    return len;
}

int route_walk_path(const Grid *g, int start, int goal, int out[], int max_len){
    int end_cell,neighbours[4],num_neigh,len=0,c,found=0;
    int *prev;
    Queue q;
    Event e={0};
    if(g==NULL || g->cells==NULL || out==NULL || max_len<1) return 0;
    end_cell=g->width*g->height;
    if(start<0 || start>=end_cell || goal<0 || goal>=end_cell) return 0;
    if(start==goal){
        out[0]=start;
        return 1;
    }
    if(!grid_is_walkable(g,goal%g->width,goal/g->width)) return 0;

    prev=(int *)malloc(end_cell*sizeof(int));
    if(prev==NULL) return 0;
    for(int i=0;i<end_cell;i++) prev[i]=-2;
    if(!queue_init(&q,end_cell)){
        free(prev);
        return 0;
    }
    prev[start]=-1;
    e.cell=start;
    queue_push(&q,e);
    while(queue_pop(&q,&e)){
        c=e.cell;
        if(c==goal){
            found=1;
            break;
        }
        num_neigh=grid_neighbours(g,c,neighbours);
        for(int k=0;k<num_neigh;k++){
            int nb=neighbours[k];
            if(prev[nb]!=-2) continue;
            if(!grid_is_walkable(g,nb%g->width,nb/g->width)) continue;
            prev[nb]=c;
            e.cell=nb;
            queue_push(&q,e);
        }
    }
    queue_free(&q);
    if(found){
        for(c=goal;c!=-1;c=prev[c]) len++;
        if(len<=max_len){
            c=goal;
            for(int i=len-1;i>=0;i--){
                out[i]=c;
                c=prev[c];
            }
        }
        else len=0;
    }
    free(prev);
    return len;
}
