#ifndef SPELL_H
#define SPELL_H

#include "grid.h"
#include "heap.h"
#include "game_config.h"

#define SPELL_MAX_RADIUS 3
#define ZONE_KIND_COUNT 4
#define ZONE_MAX 32
#define ZONE_MAX_CELLS 32
#define ZONE_GRID_CELLS (MAP_W*MAP_H)

typedef struct {
    int kind;
    int cells[ZONE_MAX_CELLS];
    int count;
    int expires;
    int active;
} Zone;

typedef struct {
    Zone zones[ZONE_MAX];
    int cover[ZONE_KIND_COUNT][ZONE_GRID_CELLS];
    int active_count;
    Heap expiry;
} ZoneTable;

int  spell_area(const Grid *g, int centre, int radius, int out[], int max_len);

void zones_clear(ZoneTable *zt);
int  zones_add(ZoneTable *zt, int kind, const int cells[], int count, int expires);
int  zones_update(ZoneTable *zt, int now);
int  zones_cover(const ZoneTable *zt, int kind, int cell);
int  zones_until(const ZoneTable *zt, int kind, int cell);
int  zones_active(const ZoneTable *zt);

#endif
