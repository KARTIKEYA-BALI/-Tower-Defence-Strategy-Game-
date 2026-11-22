#ifndef ENGINE_CORE_H
#define ENGINE_CORE_H

#include <stddef.h>
#include <stdint.h>
#include "game_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GRID_H
enum { CELL_PATH, CELL_BUILDABLE, CELL_BLOCKED, CELL_SPAWN, CELL_GOAL, CELL_OFFMAP };
#endif

#define ENGINE_MAX_SPAWNS 4

enum { THEME_MEADOW, THEME_SNOW, THEME_DESERT, THEME_AUTUMN, THEME_VOLCANIC, THEME_COUNT };
enum { ZONE_MUD, ZONE_FIRE };

int  engine_init(int width, int height);
void engine_shutdown(void);
int  engine_load_level(const char *path);
int  engine_load_level_index(int level);

int  engine_level_count(void);
int  engine_current_level(void);
const char *engine_level_name(int level);
const char *engine_level_blurb(int level);
int  engine_level_preview(int level, int x, int y);
int  engine_level_theme(int level);
int  engine_level_feature(int level, int x, int y);

int  engine_cell_type(int x, int y);
int  engine_spawn_cell(void);
int  engine_spawn_count(void);
int  engine_spawn_at(int i);
int  engine_goal_cell(void);

int  engine_can_place_tower(int x, int y);
int  engine_place_tower(int x, int y, int tower_id);
int  engine_remove_tower(int x, int y);

int  engine_next_step(int cell);
int  engine_distance_to_goal(int cell);

void engine_clear_danger(void);
int  engine_add_danger(int cell, int amount);
int  engine_danger(int cell);
int  engine_drone_path(int from, int to, int *out, int max_len);
int  engine_walk_path(int from, int to, int *out, int max_len);

int  engine_spell_area(int cell, int radius, int *out, int max_len);
int  engine_zone_add(int kind, int cell, int radius, int expires_ms);
int  engine_zone_update(int now_ms);
int  engine_zone_at(int cell, int kind);
int  engine_zone_until(int cell, int kind);
int  engine_zone_count(void);
void engine_zone_clear(void);

int  shop_init(void);
int  shop_insert(const char *name, int tower_id, int score);
int  shop_search(const char *prefix, int *out_ids, int k);
int  shop_record_purchase(const char *name);
void shop_shutdown(void);

int  map_search(const char *prefix, int *out_levels, int k);

int  save_compress(const uint8_t *in, size_t n, uint8_t **out, size_t *out_n);
int  save_decompress(const uint8_t *in, size_t n, uint8_t **out, size_t *out_n);
void save_buffer_free(uint8_t *buf);

#ifdef __cplusplus
}
#endif

#endif
