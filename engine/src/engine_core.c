#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include "grid.h"
#include "router.h"
#include "huffman.h"
#include "engine_core.h"

static const char *LEVEL[MAP_H]={
    "................",
    "................",
    "......##........",
    "................",
    "S==.............",
    "................",
    "...........##...",
    ".........==G....",
    "................",
    "..##............",
    "................",
    "..........##....",
    "................",
    "................"
};

static Grid grid;
static int ready=0;
static int spawn_cell=-1;
static int goal_cell=-1;
static int dist[MAP_W*MAP_H];
static int dist_version=-1;

static int valid_cell(int cell){
    return ready && cell>=0 && cell<MAP_W*MAP_H;
}

static void refresh_dist(void){
    if(dist_version==grid.version) return;
    route_build_distance_field(&grid,goal_cell,dist);
    dist_version=grid.version;
}

int engine_init(int width,int height){
    if(width!=MAP_W || height!=MAP_H) return 0;
    return engine_load_level(NULL);
}

void engine_shutdown(void){
    if(ready) grid_free(&grid);
    ready=0;
    spawn_cell=-1;
    goal_cell=-1;
    dist_version=-1;
}

int engine_load_level(const char *path){
    (void)path;
    engine_shutdown();
    if(!grid_init(&grid,MAP_W,MAP_H)) return 0;
    ready=1;
    for(int y=0;y<MAP_H;y++){
        for(int x=0;x<MAP_W;x++){
            char ch=LEVEL[y][x];
            if(ch=='=') grid_set_terrain(&grid,x,y,CELL_PATH);
            else if(ch=='#') grid_set_terrain(&grid,x,y,CELL_BLOCKED);
            else if(ch=='S'){
                grid_set_terrain(&grid,x,y,CELL_SPAWN);
                spawn_cell=grid_index(&grid,x,y);
            }
            else if(ch=='G'){
                grid_set_terrain(&grid,x,y,CELL_GOAL);
                goal_cell=grid_index(&grid,x,y);
            }
        }
    }
    return spawn_cell>=0 && goal_cell>=0;
}

int engine_cell_type(int x,int y){
    if(!ready) return CELL_OFFMAP;
    return grid_get_terrain(&grid,x,y);
}

int engine_spawn_cell(void){
    return spawn_cell;
}

int engine_goal_cell(void){
    return goal_cell;
}

int engine_can_place_tower(int x,int y){
    if(!ready) return 0;
    return route_can_place(&grid,x,y,spawn_cell,goal_cell);
}

int engine_place_tower(int x,int y,int tower_id){
    if(tower_id<0) return 0;
    if(!engine_can_place_tower(x,y)) return 0;
    return grid_place_tower(&grid,x,y,tower_id);
}

int engine_remove_tower(int x,int y){
    if(!ready) return 0;
    return grid_remove_tower(&grid,x,y);
}

int engine_next_step(int cell){
    if(!valid_cell(cell)) return -1;
    refresh_dist();
    return route_next_step(&grid,dist,cell);
}

int engine_distance_to_goal(int cell){
    if(!valid_cell(cell)) return -1;
    refresh_dist();
    return dist[cell];
}

int save_compress(const uint8_t *in,size_t n,uint8_t **out,size_t *out_n){
    if(out==NULL || out_n==NULL) return 0;
    return huffman_compress(in,n,out,out_n);
}

int save_decompress(const uint8_t *in,size_t n,uint8_t **out,size_t *out_n){
    if(out==NULL || out_n==NULL) return 0;
    return huffman_decompress(in,n,out,out_n);
}

void save_buffer_free(uint8_t *buf){
    huffman_free(buf);
}
