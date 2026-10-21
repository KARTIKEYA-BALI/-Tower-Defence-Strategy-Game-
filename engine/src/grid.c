#include <stdlib.h>
# include "grid.h"


int grid_init(Grid * g, int width, int height ){

    if(width<=0 || height<=0) {
        g->cells=NULL ;
        g->width=0 ;
        g->height=0 ;
        return 0 ;
    }
    g->width=width ;
    g->height=height ;
    g->version=0; 
    g->cells=(Cell *)malloc(sizeof(Cell)*width*height) ;
    if(g->cells==NULL) return 0; 

    for (int i=0 ;i < width*height ;i++){
        g->cells[i].terrain=CELL_BUILDABLE;
        g->cells[i].tower_id=-1; 
        g->cells[i].danger=0;
    }

    return 1; 
}

int grid_in_bounds(const Grid *g ,int x ,int y) {
    if((x>=0 && x<g->width) && (y>=0 && y<g->height))return 1; 
    else return  0; 
}

int grid_index(const Grid *g ,int x ,int y) {
    int index=-1; 
    if(grid_in_bounds( g , x , y)){
        index=y*g->width+x;
    }
    return index; 
}


CellType grid_get_terrain(const Grid * g, int x ,int y){
    if(grid_in_bounds(g,x,y)) {
        return g->cells[grid_index(g,x,y)].terrain; 
    }
    return CELL_OFFMAP; 
}

int grid_set_terrain(Grid *g, int x, int y, CellType t){
        if(t==CELL_OFFMAP) return 0 ;
        int index=grid_index(g,x,y);
        if(index==-1) return 0;
        g->version ++ ;
        g->cells[index].terrain=t ;
        return 1;
}

void grid_free(Grid *g){
    free(g->cells) ; 
    g->cells=NULL ;
    g->width=0 ;
    g->height=0 ;
}


int grid_place_tower(Grid *g, int x, int y, int t_id){
    if(t_id<0) return 0; 
    if(grid_in_bounds(g,x,y)){
        int index=grid_index(g,x,y) ;
        if(g->cells[index].terrain==CELL_BUILDABLE && g->cells[index].tower_id==-1){
            g->cells[index].tower_id=t_id;
            g->version++ ;
            return 1 ;
        }
        else return 0; 
    }
    else return 0 ;


}


int grid_remove_tower(Grid *g, int x, int y){
    if(grid_in_bounds(g,x,y)){
        int index=grid_index(g,x,y) ;
        if((g->cells[index].tower_id)>=0){
            g->cells[index].tower_id=-1;
            g->version++; 
            return 1; 
        }
        else return 0; 
    }
    else return 0 ;
}

int grid_neighbours(const Grid * g, int cell , int out[4]){
    int i=0 ;
    if(cell<0 || cell >= g->height*g->width) return 0 ;
    int x=cell%g->width;
    int y =cell/g->width;
    
    if(grid_in_bounds(g,x,y)){
        //upper neighbour 
        if(grid_in_bounds(g,x,y-1)){
            out[i]=grid_index(g,x,y-1);i++;
        }
        //lower neighbour 
        if(grid_in_bounds(g,x,y+1)){
            out[i]=grid_index(g,x,y+1);i++;
        }
        //left neighbour 
        if(grid_in_bounds(g,x-1,y)){
            out[i]=grid_index(g,x-1,y);i++;
        }
        //right neighbour
        if(grid_in_bounds(g,x+1,y)){
            out[i]=grid_index(g,x+1,y);i++;
        }
        return i; 
    }
    return 0; 
    
}

int grid_is_walkable(const Grid *g, int x, int y){
    if(grid_in_bounds(g,x,y) && (g->cells[grid_index(g,x,y)].terrain!=CELL_BLOCKED) && g->cells[grid_index(g,x,y)].tower_id==-1){
        return 1; 
    }
    else return 0 ;
}
