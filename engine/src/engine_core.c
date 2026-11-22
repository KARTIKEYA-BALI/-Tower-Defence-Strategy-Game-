#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include "grid.h"
#include "router.h"
#include "huffman.h"
#include "search.h"
#include "spell.h"
#include "engine_core.h"

typedef struct {
    const char *name;
    const char *blurb;
    int theme;
    const char *rows[MAP_H];
} Level;

static const Level LEVELS[]={
    {"Greenfield","Open meadow and a single burrow. The classic siege.",THEME_MEADOW,{
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
    }},
    {"Snowy Valley","Two burrows high in the pines. The castle waits at the bottom of the valley.",THEME_SNOW,{
        "TTTT........TTTT",
        "TTT....##....TTT",
        "S==..........==S",
        "TT............TT",
        "T....TT..TT....T",
        "......T..T......",
        "..#..........#..",
        "T......TT......T",
        "TT............TT",
        "T.....#..#.....T",
        "......==G==.....",
        "TT............TT",
        "TTT..........TTT",
        "TTTT..T..T..TTTT"
    }},
    {"Desert Canyon","Sandstone boulders and cacti scattered over the dunes, and a burrow hidden in the rocks.",THEME_DESERT,{
        ".......T..##....",
        ".........#.T.==S",
        "...#.T..........",
        "..#...#.........",
        ".T#...#.......#.",
        ".....T.#....T...",
        ".......#....T...",
        ".G==.......T....",
        ".....TT......#..",
        "....#..#....#...",
        "............#.T#",
        ".......#........",
        "...T...#.....###",
        "##.........T..S#"
    }},
    {"Autumn Forest","Thick woods and winding trails. Little room to build, so choose well.",THEME_AUTUMN,{
        "TT...TTTT...TTTT",
        "T.....TT.....TTT",
        "....S==......TT.",
        "..TT......TT....",
        ".TTT..#...TT....",
        "......#.........",
        "TT..........TT..",
        "T....TTT....==G.",
        "S==.....T.......",
        "TT...#......TT..",
        "T....#..TT...TT.",
        "...TT...TT......",
        "TT..........TTTT",
        "TTTT..TTT..TTTTT"
    }},
    {"Volcanic Ridge","Three burrows and rivers of lava. The hardest siege of all.",THEME_VOLCANIC,{
        "S==.....T.....TT",
        "........T......S",
        "..TTT...T..#....",
        "....T.......#...",
        ".#..T...TTTT....",
        "....T...........",
        "........G...TT..",
        "..TTTTT.........",
        "......T....#....",
        ".#....T..TTTTT..",
        "......T.........",
        "...#.....T...#..",
        "TT.......T......",
        "S==TT....T...TTT"
    }}
};

#define LEVEL_COUNT ((int)(sizeof(LEVELS)/sizeof(LEVELS[0])))

static Grid grid;
static int ready=0;
static int level_index=-1;
static int spawn_cells[ENGINE_MAX_SPAWNS];
static int spawn_count=0;
static int goal_cell=-1;
static int dist[MAP_W*MAP_H];
static int dist_version=-1;
static ZoneTable zones;
static Trie shop_trie;
static Trie map_trie;
static int map_index_ready=0;

static int valid_cell(int cell){
    return ready && cell>=0 && cell<MAP_W*MAP_H;
}

static void refresh_dist(void){
    if(dist_version==grid.version) return;
    route_build_distance_field(&grid,goal_cell,dist);
    dist_version=grid.version;
}

static int terrain_of(char ch){
    if(ch=='=') return CELL_PATH;
    if(ch=='#' || ch=='T') return CELL_BLOCKED;
    if(ch=='S') return CELL_SPAWN;
    if(ch=='G') return CELL_GOAL;
    return CELL_BUILDABLE;
}

int engine_init(int width,int height){
    if(width!=MAP_W || height!=MAP_H) return 0;
    return engine_load_level(NULL);
}

void engine_shutdown(void){
    if(ready) grid_free(&grid);
    ready=0;
    level_index=-1;
    spawn_count=0;
    goal_cell=-1;
    dist_version=-1;
    zones_clear(&zones);
}

int engine_load_level(const char *path){
    (void)path;
    return engine_load_level_index(0);
}

int engine_load_level_index(int level){
    if(level<0 || level>=LEVEL_COUNT) return 0;
    engine_shutdown();
    if(!grid_init(&grid,MAP_W,MAP_H)) return 0;
    ready=1;
    level_index=level;
    for(int y=0;y<MAP_H;y++){
        for(int x=0;x<MAP_W;x++){
            int t=terrain_of(LEVELS[level].rows[y][x]);
            if(t!=CELL_BUILDABLE) grid_set_terrain(&grid,x,y,(CellType)t);
            if(t==CELL_SPAWN && spawn_count<ENGINE_MAX_SPAWNS) spawn_cells[spawn_count++]=grid_index(&grid,x,y);
            if(t==CELL_GOAL) goal_cell=grid_index(&grid,x,y);
        }
    }
    return spawn_count>0 && goal_cell>=0;
}

int engine_level_count(void){
    return LEVEL_COUNT;
}

int engine_current_level(void){
    return level_index;
}

const char *engine_level_name(int level){
    if(level<0 || level>=LEVEL_COUNT) return "";
    return LEVELS[level].name;
}

const char *engine_level_blurb(int level){
    if(level<0 || level>=LEVEL_COUNT) return "";
    return LEVELS[level].blurb;
}

int engine_level_theme(int level){
    if(level<0 || level>=LEVEL_COUNT) return THEME_MEADOW;
    return LEVELS[level].theme;
}

int engine_level_feature(int level,int x,int y){
    if(level<0 || level>=LEVEL_COUNT || x<0 || x>=MAP_W || y<0 || y>=MAP_H) return 0;
    return LEVELS[level].rows[y][x]=='T';
}

int engine_level_preview(int level,int x,int y){
    if(level<0 || level>=LEVEL_COUNT || x<0 || x>=MAP_W || y<0 || y>=MAP_H) return CELL_OFFMAP;
    return terrain_of(LEVELS[level].rows[y][x]);
}

int engine_cell_type(int x,int y){
    if(!ready) return CELL_OFFMAP;
    return grid_get_terrain(&grid,x,y);
}

int engine_spawn_cell(void){
    return spawn_count>0 ? spawn_cells[0] : -1;
}

int engine_spawn_count(void){
    return spawn_count;
}

int engine_spawn_at(int i){
    if(i<0 || i>=spawn_count) return -1;
    return spawn_cells[i];
}

int engine_goal_cell(void){
    return goal_cell;
}

int engine_can_place_tower(int x,int y){
    if(!ready || spawn_count==0) return 0;
    for(int i=0;i<spawn_count;i++){
        if(!route_can_place(&grid,x,y,spawn_cells[i],goal_cell)) return 0;
    }
    return 1;
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

void engine_clear_danger(void){
    if(ready) grid_clear_danger(&grid);
}

int engine_add_danger(int cell,int amount){
    if(!valid_cell(cell) || amount<=0) return 0;
    return grid_set_danger(&grid,cell,grid.cells[cell].danger+amount);
}

int engine_danger(int cell){
    if(!valid_cell(cell)) return 0;
    return grid.cells[cell].danger;
}

int engine_drone_path(int from,int to,int *out,int max_len){
    if(!valid_cell(from) || !valid_cell(to) || out==NULL) return 0;
    return route_drone_path(&grid,from,to,out,max_len);
}

int engine_walk_path(int from,int to,int *out,int max_len){
    if(!valid_cell(from) || !valid_cell(to) || out==NULL) return 0;
    return route_walk_path(&grid,from,to,out,max_len);
}

int engine_spell_area(int cell,int radius,int *out,int max_len){
    if(!valid_cell(cell) || out==NULL) return 0;
    return spell_area(&grid,cell,radius,out,max_len);
}

int engine_zone_add(int kind,int cell,int radius,int expires_ms){
    int cells[ZONE_MAX_CELLS];
    int n=engine_spell_area(cell,radius,cells,ZONE_MAX_CELLS);
    if(n==0) return -1;
    return zones_add(&zones,kind,cells,n,expires_ms);
}

int engine_zone_update(int now_ms){
    return zones_update(&zones,now_ms);
}

int engine_zone_at(int cell,int kind){
    return zones_cover(&zones,kind,cell);
}

int engine_zone_until(int cell,int kind){
    return zones_until(&zones,kind,cell);
}

int engine_zone_count(void){
    return zones_active(&zones);
}

void engine_zone_clear(void){
    zones_clear(&zones);
}

int shop_init(void){
    return trie_init(&shop_trie);
}

int shop_insert(const char *name,int tower_id,int score){
    return trie_insert(&shop_trie,name,tower_id,score);
}

int shop_search(const char *prefix,int *out_ids,int k){
    if(prefix==NULL || out_ids==NULL) return 0;
    return trie_search(&shop_trie,prefix,out_ids,k);
}

int shop_record_purchase(const char *name){
    return trie_add_score(&shop_trie,name,SHOP_PURCHASE_BONUS);
}

void shop_shutdown(void){
    trie_shutdown(&shop_trie);
}

static int build_map_index(void){
    char word[TRIE_MAX_NAME];
    trie_init(&map_trie);
    for(int lv=0;lv<LEVEL_COUNT;lv++){
        const char *name=LEVELS[lv].name;
        int score=LEVEL_COUNT-lv;
        if(!trie_insert(&map_trie,name,lv,score)) return 0;
        for(int i=0;name[i]!='\0';i++){
            if(name[i]!=' ') continue;
            int len=0;
            for(int j=i+1;name[j]!='\0' && len<TRIE_MAX_NAME-1;j++) word[len++]=name[j];
            word[len]='\0';
            if(len>0) trie_insert(&map_trie,word,lv,score);
        }
    }
    return 1;
}

int map_search(const char *prefix,int *out_levels,int k){
    if(prefix==NULL || out_levels==NULL) return 0;
    if(!map_index_ready){
        if(!build_map_index()) return 0;
        map_index_ready=1;
    }
    return trie_search(&map_trie,prefix,out_levels,k);
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
