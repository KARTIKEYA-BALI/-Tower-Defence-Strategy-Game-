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

int  engine_init(int width, int height);
void engine_shutdown(void);
int  engine_load_level(const char *path);

int  engine_cell_type(int x, int y);
int  engine_spawn_cell(void);
int  engine_goal_cell(void);

int  engine_can_place_tower(int x, int y);
int  engine_place_tower(int x, int y, int tower_id);
int  engine_remove_tower(int x, int y);

int  engine_next_step(int cell);
int  engine_distance_to_goal(int cell);

int  shop_init(void);
int  shop_insert(const char *name, int tower_id, int score);
int  shop_search(const char *prefix, int *out_ids, int k);
int  shop_record_purchase(const char *name);
void shop_shutdown(void);

int  save_compress(const uint8_t *in, size_t n, uint8_t **out, size_t *out_n);
int  save_decompress(const uint8_t *in, size_t n, uint8_t **out, size_t *out_n);
void save_buffer_free(uint8_t *buf);

#ifdef __cplusplus
}
#endif

#endif
