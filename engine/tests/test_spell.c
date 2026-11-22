#include <stdio.h>
#include <string.h>
#include "spell.h"
#include "engine_core.h"

static int passed = 0;
static int failed = 0;
static ZoneTable zt;

static void check(int condition, const char *name)
{
    if (condition) {
        printf("PASS  %s\n", name);
        passed++;
    } else {
        printf("FAIL  %s\n", name);
        failed++;
    }
}

static int contains(const int *cells, int n, int cell)
{
    for (int i = 0; i < n; i++) {
        if (cells[i] == cell) return 1;
    }
    return 0;
}

static int all_within(const int *cells, int n, int centre, int radius, int width)
{
    int cx = centre % width;
    int cy = centre / width;
    for (int i = 0; i < n; i++) {
        int dx = cells[i] % width - cx;
        int dy = cells[i] / width - cy;
        if (dx < 0) dx = -dx;
        if (dy < 0) dy = -dy;
        if (dx + dy > radius) return 0;
    }
    return 1;
}

static void test_area(void)
{
    Grid g;
    int out[64];
    int n;

    grid_init(&g, 7, 7);
    n = spell_area(&g, 24, 0, out, 64);
    check(n == 1 && out[0] == 24, "radius 0 is just the centre");
    n = spell_area(&g, 24, 1, out, 64);
    check(n == 5 && all_within(out, n, 24, 1, 7), "radius 1 in open ground is a plus of 5 cells");
    n = spell_area(&g, 24, 2, out, 64);
    check(n == 13 && all_within(out, n, 24, 2, 7), "radius 2 in open ground is a diamond of 13 cells");
    n = spell_area(&g, 24, 3, out, 64);
    check(n == 25 && all_within(out, n, 24, 3, 7), "radius 3 in open ground is 25 cells");
    check(out[0] == 24, "the centre comes first");
    n = spell_area(&g, 0, 2, out, 64);
    check(n == 6 && contains(out, n, 0) && contains(out, n, 2) && contains(out, n, 14) && contains(out, n, 8),
          "the map edge cuts the area");

    grid_set_terrain(&g, 3, 2, CELL_BLOCKED);
    grid_set_terrain(&g, 2, 3, CELL_BLOCKED);
    grid_set_terrain(&g, 4, 3, CELL_BLOCKED);
    grid_set_terrain(&g, 3, 4, CELL_BLOCKED);
    n = spell_area(&g, 24, 3, out, 64);
    check(n == 1, "rocks all round: the spell stays on its own cell");
    n = spell_area(&g, 17, 1, out, 64);
    check(n == 0, "casting on a rock is refused");

    grid_free(&g);
    grid_init(&g, 7, 7);
    for (int y = 0; y < 6; y++) grid_set_terrain(&g, 3, y, CELL_BLOCKED);
    n = spell_area(&g, 2 * 7 + 2, 2, out, 64);
    check(!contains(out, n, 2 * 7 + 4), "the spell does not jump over a wall");
    n = spell_area(&g, 5 * 7 + 2, 3, out, 64);
    check(contains(out, n, 6 * 7 + 3) && contains(out, n, 6 * 7 + 4) && !contains(out, n, 5 * 7 + 4),
          "it spreads round the end of the wall, step by step");

    grid_place_tower(&g, 1, 1, 0);
    n = spell_area(&g, 1 * 7 + 1, 1, out, 64);
    check(n == 5, "casting on a tower is allowed and spreads round it");
    n = spell_area(&g, 1 * 7 + 0, 1, out, 64);
    check(!contains(out, n, 1 * 7 + 1), "a tower cell next to the centre is not covered");

    n = spell_area(&g, 6 * 7 + 0, 3, out, 3);
    check(n == 3, "the output buffer is never overrun");
    check(spell_area(&g, -1, 1, out, 64) == 0 && spell_area(&g, 49, 1, out, 64) == 0, "centre off the grid refused");
    check(spell_area(&g, 42, -1, out, 64) == 0 && spell_area(&g, 42, SPELL_MAX_RADIUS + 1, out, 64) == 0,
          "radius out of range refused");
    check(spell_area(NULL, 42, 1, out, 64) == 0 && spell_area(&g, 42, 1, NULL, 64) == 0, "NULL refused");
    grid_free(&g);
}

static void test_zones(void)
{
    int a[3] = {10, 11, 12};
    int b[2] = {11, 30};
    int bad[1] = {ZONE_GRID_CELLS};

    zones_clear(&zt);
    check(zones_active(&zt) == 0, "new table is empty");
    check(zones_add(&zt, ZONE_MUD, a, 3, 5000) >= 0, "add a mud zone until 5000");
    check(zones_add(&zt, ZONE_MUD, b, 2, 2000) >= 0, "add a second mud zone until 2000");
    check(zones_add(&zt, ZONE_FIRE, b, 2, 3000) >= 0, "add a fire zone until 3000");
    check(zones_active(&zt) == 3, "three zones active");
    check(zones_cover(&zt, ZONE_MUD, 11) == 2, "overlapping mud counts twice");
    check(zones_cover(&zt, ZONE_FIRE, 11) == 1 && zones_cover(&zt, ZONE_FIRE, 10) == 0, "kinds are kept apart");
    check(zones_until(&zt, ZONE_MUD, 11) == 5000, "until gives the latest end time on a cell");
    check(zones_until(&zt, ZONE_MUD, 99) == -1, "until on an empty cell is -1");

    check(zones_update(&zt, 1999) == 0, "nothing ends before its time");
    check(zones_update(&zt, 2000) == 1 && zones_cover(&zt, ZONE_MUD, 30) == 0 && zones_cover(&zt, ZONE_MUD, 11) == 1,
          "the earliest zone ends first");
    check(zones_update(&zt, 10000) == 2 && zones_active(&zt) == 0, "a late update ends every zone that is due");
    check(zones_cover(&zt, ZONE_MUD, 10) == 0 && zones_cover(&zt, ZONE_FIRE, 11) == 0, "all cover is gone");

    check(zones_add(&zt, -1, a, 3, 10) == -1 && zones_add(&zt, ZONE_KIND_COUNT, a, 3, 10) == -1, "bad kind refused");
    check(zones_add(&zt, ZONE_MUD, a, 0, 10) == -1 && zones_add(&zt, ZONE_MUD, a, ZONE_MAX_CELLS + 1, 10) == -1,
          "bad cell count refused");
    check(zones_add(&zt, ZONE_MUD, bad, 1, 10) == -1, "cell off the grid refused");

    int filled = 0;
    for (int i = 0; i < ZONE_MAX + 5; i++) {
        if (zones_add(&zt, ZONE_FIRE, a, 1, 100 + i) >= 0) filled++;
    }
    check(filled == ZONE_MAX, "the table holds at most ZONE_MAX zones");
    check(zones_update(&zt, 100) == 1 && zones_add(&zt, ZONE_FIRE, a, 1, 900) >= 0, "a freed slot is used again");
    zones_clear(&zt);
    check(zones_active(&zt) == 0 && zones_cover(&zt, ZONE_FIRE, 10) == 0, "clear empties everything");
}

static void test_engine_spells(void)
{
    int out[32];
    int n;
    engine_load_level_index(0);
    int goal = engine_goal_cell();
    n = engine_spell_area(goal, 2, out, 32);
    check(n > 0 && out[0] == goal, "engine: area round the castle");
    int rock = 2 * MAP_W + 6;
    check(engine_cell_type(6, 2) == CELL_BLOCKED && engine_spell_area(rock, 1, out, 32) == 0, "engine: rock refused");

    int cell = 5 * MAP_W + 5;
    check(engine_zone_add(ZONE_MUD, cell, 2, 4000) >= 0, "engine: add mud");
    check(engine_zone_add(ZONE_FIRE, cell, 1, 2000) >= 0, "engine: add fire");
    check(engine_zone_count() == 2, "engine: two zones");
    check(engine_zone_at(cell + 2, ZONE_MUD) == 1 && engine_zone_at(cell + 2, ZONE_FIRE) == 0,
          "engine: radius 2 mud reaches 2 cells, radius 1 fire does not");
    check(engine_zone_until(cell, ZONE_FIRE) == 2000, "engine: fire end time");
    check(engine_zone_update(2500) == 1 && engine_zone_at(cell, ZONE_FIRE) == 0 && engine_zone_at(cell, ZONE_MUD) == 1,
          "engine: fire ends, mud stays");
    check(engine_zone_add(ZONE_MUD, rock, 1, 9000) == -1, "engine: zone on a rock refused");
    engine_load_level_index(1);
    check(engine_zone_count() == 0 && engine_zone_at(cell, ZONE_MUD) == 0, "engine: loading a level clears zones");
    engine_zone_add(ZONE_MUD, engine_goal_cell(), 1, 100);
    engine_zone_clear();
    check(engine_zone_count() == 0, "engine: clear");
    engine_shutdown();
    check(engine_spell_area(cell, 1, out, 32) == 0 && engine_zone_add(ZONE_MUD, cell, 1, 10) == -1,
          "engine: nothing works after shutdown");
}

int main(void)
{
    test_area();
    test_zones();
    test_engine_spells();
    printf("\n%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
