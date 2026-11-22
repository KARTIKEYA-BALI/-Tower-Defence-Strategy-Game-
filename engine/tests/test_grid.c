
#include <stdio.h>
#include "grid.h"

static int passed = 0;
static int failed = 0;

static void check(int condition, const char *name) {
    if (condition) {
        printf("PASS: %s\n", name);
        passed++;
    } else {
        printf("FAIL: %s\n", name);
        failed++;
    }
}

static int has(const int out[4], int count, int value) {
    for (int i = 0; i < count; i++) {
        if (out[i] == value) return 1;
    }
    return 0;
}

int main(void) {
    Grid g;
    int out[4];
    int n;
    int v;
    int ok;

    check(grid_init(&g, 0, 3) == 0, "init rejects width 0");
    check(grid_init(&g, 4, 0) == 0, "init rejects height 0");
    check(grid_init(&g, -4, 3) == 0, "init rejects negative width");
    check(g.cells == NULL && g.width == 0 && g.height == 0, "failed init leaves grid empty");
    grid_free(&g);
    check(1, "free after failed init does not crash");

    check(grid_init(&g, 4, 3) == 1, "init 4x3 succeeds");
    check(g.width == 4 && g.height == 3 && g.version == 0, "new grid has width 4, height 3, version 0");
    ok = 1;
    for (int i = 0; i < 12; i++) {
        if (g.cells[i].terrain != CELL_BUILDABLE || g.cells[i].tower_id != -1 || g.cells[i].danger != 0) ok = 0;
    }
    check(ok, "every new cell is buildable, no tower, danger 0");

    check(grid_index(&g, 0, 0) == 0, "index (0,0) is 0");
    check(grid_index(&g, 2, 1) == 6, "index (2,1) is 6");
    check(grid_index(&g, 3, 2) == 11, "index (3,2) is 11");
    check(6 % g.width == 2 && 6 / g.width == 1, "cell 6 converts back to (2,1)");
    check(grid_index(&g, 4, 0) == -1, "index off the map is -1");

    check(grid_in_bounds(&g, 0, 0), "in bounds (0,0)");
    check(grid_in_bounds(&g, 3, 0), "in bounds (3,0)");
    check(grid_in_bounds(&g, 0, 2), "in bounds (0,2)");
    check(grid_in_bounds(&g, 3, 2), "in bounds (3,2)");
    check(!grid_in_bounds(&g, -1, 0), "not in bounds (-1,0)");
    check(!grid_in_bounds(&g, 4, 0), "not in bounds (4,0) x == width");
    check(!grid_in_bounds(&g, 0, -1), "not in bounds (0,-1)");
    check(!grid_in_bounds(&g, 0, 3), "not in bounds (0,3) y == height");

    check(grid_get_terrain(&g, 1, 1) == CELL_BUILDABLE, "get terrain of new cell is buildable");
    check(grid_get_terrain(&g, -1, 0) == CELL_OFFMAP, "get terrain left of map is OFFMAP");
    check(grid_get_terrain(&g, 4, 0) == CELL_OFFMAP, "get terrain right of map is OFFMAP");
    check(grid_get_terrain(&g, 0, -1) == CELL_OFFMAP, "get terrain above map is OFFMAP");
    check(grid_get_terrain(&g, 0, 3) == CELL_OFFMAP, "get terrain below map is OFFMAP");

    check(grid_set_terrain(&g, 2, 1, CELL_BLOCKED) == 1, "set (2,1) to blocked");
    check(grid_get_terrain(&g, 2, 1) == CELL_BLOCKED, "(2,1) is now blocked");
    check(g.version == 1, "set terrain bumps version to 1");
    check(g.cells[6].tower_id == -1 && g.cells[6].danger == 0, "set terrain leaves tower_id and danger alone");
    v = g.version;
    check(grid_set_terrain(&g, 4, 0, CELL_PATH) == 0, "set terrain off the map fails");
    check(grid_set_terrain(&g, 1, 1, CELL_OFFMAP) == 0, "set terrain to OFFMAP is refused");
    check(g.version == v, "failed set terrain does not bump version");
    check(grid_get_terrain(&g, 1, 1) == CELL_BUILDABLE, "refused OFFMAP leaves cell buildable");

    grid_set_terrain(&g, 0, 0, CELL_SPAWN);
    grid_set_terrain(&g, 3, 2, CELL_GOAL);
    grid_set_terrain(&g, 0, 1, CELL_PATH);

    v = g.version;
    check(grid_place_tower(&g, 1, 1, 7) == 1, "place tower 7 on buildable (1,1)");
    check(g.cells[5].tower_id == 7, "(1,1) now holds tower 7");
    check(g.version == v + 1, "place tower bumps version");
    check(grid_get_terrain(&g, 1, 1) == CELL_BUILDABLE, "terrain under tower is still buildable");
    v = g.version;
    check(grid_place_tower(&g, 1, 1, 9) == 0, "place on occupied cell fails");
    check(g.cells[5].tower_id == 7, "occupied cell keeps tower 7");
    check(grid_place_tower(&g, 0, 1, 3) == 0, "place on path fails");
    check(grid_place_tower(&g, 2, 1, 3) == 0, "place on blocked fails");
    check(grid_place_tower(&g, 0, 0, 3) == 0, "place on spawn fails");
    check(grid_place_tower(&g, 3, 2, 3) == 0, "place on goal fails");
    check(grid_place_tower(&g, 4, 0, 3) == 0, "place off the map fails");
    check(grid_place_tower(&g, 2, 2, -1) == 0, "place with tower_id -1 fails");
    check(grid_place_tower(&g, 2, 2, -5) == 0, "place with negative tower_id fails");
    check(g.version == v, "failed placements do not bump version");

    v = g.version;
    check(grid_remove_tower(&g, 1, 1) == 1, "remove tower at (1,1)");
    check(g.cells[5].tower_id == -1, "(1,1) has no tower after remove");
    check(g.version == v + 1, "remove tower bumps version");
    v = g.version;
    check(grid_remove_tower(&g, 1, 1) == 0, "second remove fails");
    check(grid_remove_tower(&g, 2, 2) == 0, "remove on empty cell fails");
    check(grid_remove_tower(&g, -1, 0) == 0, "remove off the map fails");
    check(g.version == v, "failed removes do not bump version");
    check(grid_place_tower(&g, 1, 1, 8) == 1, "can place again after remove");

    check(grid_is_walkable(&g, 2, 2), "buildable empty cell is walkable");
    check(grid_is_walkable(&g, 0, 1), "path is walkable");
    check(grid_is_walkable(&g, 0, 0), "spawn is walkable");
    check(grid_is_walkable(&g, 3, 2), "goal is walkable");
    check(!grid_is_walkable(&g, 2, 1), "blocked cell is not walkable");
    check(!grid_is_walkable(&g, 1, 1), "tower cell is not walkable");
    check(!grid_is_walkable(&g, 4, 0), "off the map is not walkable");
    check(grid_get_terrain(&g, 0, 0) == CELL_SPAWN && grid_get_terrain(&g, 2, 1) == CELL_BLOCKED,
          "is_walkable does not change terrain");

    n = grid_neighbours(&g, 5, out);
    check(n == 4 && has(out, n, 1) && has(out, n, 9) && has(out, n, 4) && has(out, n, 6),
          "middle cell 5 has neighbours 1, 9, 4, 6");
    n = grid_neighbours(&g, 0, out);
    check(n == 2 && has(out, n, 1) && has(out, n, 4), "corner cell 0 has neighbours 1, 4");
    n = grid_neighbours(&g, 3, out);
    check(n == 2 && has(out, n, 2) && has(out, n, 7), "corner cell 3 has neighbours 2, 7");
    check(!has(out, n, 4), "cell 3 does not wrap to cell 4");
    n = grid_neighbours(&g, 4, out);
    check(n == 3 && has(out, n, 0) && has(out, n, 8) && has(out, n, 5), "edge cell 4 has neighbours 0, 8, 5");
    check(!has(out, n, 3), "cell 4 does not wrap to cell 3");
    n = grid_neighbours(&g, 1, out);
    check(n == 3 && has(out, n, 0) && has(out, n, 2) && has(out, n, 5), "edge cell 1 has neighbours 0, 2, 5");
    n = grid_neighbours(&g, 11, out);
    check(n == 2 && has(out, n, 7) && has(out, n, 10), "corner cell 11 has neighbours 7, 10");
    check(grid_neighbours(&g, -1, out) == 0, "cell -1 has no neighbours");
    check(grid_neighbours(&g, 12, out) == 0, "cell 12 (past the end) has no neighbours");

    grid_free(&g);
    check(g.cells == NULL && g.width == 0 && g.height == 0, "free sets cells NULL and size 0");
    check(!grid_in_bounds(&g, 0, 0), "nothing is in bounds after free");
    check(grid_neighbours(&g, 0, out) == 0, "neighbours after free returns 0 without crashing");
    grid_free(&g);
    check(1, "second free does not crash");

    check(grid_init(&g, 16, 14) == 1, "init game map 16x14");
    ok = 1;
    for (int i = 0; i < 16 * 14; i++) {
        if (g.cells[i].terrain != CELL_BUILDABLE || g.cells[i].tower_id != -1) ok = 0;
    }
    check(ok, "all 224 cells of the game map start buildable and empty");
    check(grid_index(&g, 15, 13) == 223, "last cell of the game map is 223");
    grid_free(&g);

    printf("\n%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}