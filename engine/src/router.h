#include "grid.h"
#ifndef ROUTER_H
#define ROUTER_H
#define ROUTE_UNREACHABLE -1
int route_path_exists(const Grid *g,int start ,int goal) ;
int route_can_place(Grid *g,int x , int y ,int start ,int goal); 
int route_build_distance_field(const Grid *g, int goal , int dist[]) ;
int route_next_step(const Grid *g, const int dist[], int cell);

#endif 