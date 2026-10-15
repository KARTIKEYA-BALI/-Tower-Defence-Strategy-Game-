#ifndef GRID_H 
#define GRID_H

typedef enum{
    CELL_PATH,
    CELL_BUILDABLE,
    CELL_BLOCKED,
    CELL_SPAWN,
    CELL_GOAL,
    CELL_OFFMAP
}CellType; 


typedef struct {
    CellType terrain; 
    int tower_id; 
    int danger ; 
}Cell;


typedef struct {
    int width ; 
    int height ;
    Cell *cells; 
    int version ; 
}Grid;


int grid_init(Grid * g, int width, int height ); 

int grid_set_terrain(Grid *g, int x, int y, CellType t);

void grid_free(Grid *g);

int grid_place_tower(Grid *g, int x, int y, int tower_id);

int grid_in_bounds(const Grid *g ,int x ,int y) ;

int grid_index(const Grid *g ,int x ,int y) ;

CellType grid_get_terrain(const Grid * g, int x ,int y); 

int grid_remove_tower(Grid*g,int x ,int y ) ;

int grid_is_walkable(const Grid *g ,int x, int y) ;

int grid_neighbours(const Grid * g, int cell , int out[4]);

#endif
