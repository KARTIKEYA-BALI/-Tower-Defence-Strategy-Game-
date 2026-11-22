#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "grid.h"
#include "router.h"
#include "engine_core.h"

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

static int adjacent(int a, int b, int w) {
    int ax = a % w, ay = a / w, bx = b % w, by = b / w;
    return abs(ax - bx) + abs(ay - by) == 1;
}

static int valid_path(const int *p, int len, int start, int goal, int w) {
    if (len < 1 || p[0] != start || p[len - 1] != goal) return 0;
    for (int i = 1; i < len; i++) {
        if (!adjacent(p[i - 1], p[i], w)) return 0;
    }
    return 1;
}

static void test_drone_router(void) {
    Grid g;
    int path[64];
    int len;

    grid_init(&g, 5, 5);
    len = route_drone_path(&g, 0, 24, path, 64);
    check(len == 9, "open 5x5: corner to corner is 9 cells");
    check(valid_path(path, len, 0, 24, 5), "open 5x5: path starts, ends and steps correctly");
    len = route_drone_path(&g, 12, 12, path, 64);
    check(len == 1 && path[0] == 12, "start equals goal gives one cell");
    check(route_drone_path(&g, 0, 24, path, 8) == 0, "path longer than the buffer is refused");
    check(route_drone_path(&g, -1, 24, path, 64) == 0, "negative start refused");
    check(route_drone_path(&g, 0, 25, path, 64) == 0, "goal off the grid refused");
    check(route_drone_path(&g, 0, 24, path, 0) == 0, "zero buffer refused");
    grid_free(&g);

    grid_init(&g, 5, 1);
    grid_place_tower(&g, 2, 0, 7);
    check(route_path_exists(&g, 0, 4) == 0, "corridor with tower: enemies cannot pass");
    len = route_drone_path(&g, 0, 4, path, 64);
    check(len == 5 && valid_path(path, len, 0, 4, 5), "corridor with tower: drones fly over it");
    grid_free(&g);

    grid_init(&g, 5, 1);
    grid_set_terrain(&g, 2, 0, CELL_BLOCKED);
    check(route_drone_path(&g, 0, 4, path, 64) == 0, "corridor with rock: drones cannot pass");
    check(route_drone_path(&g, 2, 4, path, 64) == 0, "start on a rock refused");
    grid_free(&g);

    grid_init(&g, 3, 2);
    len = route_drone_path(&g, 0, 2, path, 64);
    check(len == 3 && path[1] == 1, "no danger: short route straight across");
    int before = g.version;
    grid_set_danger(&g, 1, 1);
    check(g.version == before, "setting danger does not change the grid version");
    len = route_drone_path(&g, 0, 2, path, 64);
    check(len == 5 && valid_path(path, len, 0, 2, 3) && path[1] == 3 && path[2] == 4 && path[3] == 5,
          "danger 1: long quiet route (cost 4) beats short risky one (cost 8)");
    grid_clear_danger(&g);
    len = route_drone_path(&g, 0, 2, path, 64);
    check(len == 3, "after clearing danger the short route is back");
    grid_free(&g);

    grid_init(&g, 5, 1);
    grid_set_danger(&g, 2, 3);
    len = route_drone_path(&g, 0, 4, path, 64);
    check(len == 5, "danger with no way round: still flies through");
    check(grid_set_danger(&g, 9, 1) == 0 && grid_set_danger(&g, -1, 1) == 0, "danger outside the grid refused");
    grid_set_danger(&g, 1, -4);
    check(g.cells[1].danger == 0, "negative danger becomes 0");
    grid_free(&g);
}

static int count_type(int level, int type) {
    int n = 0;
    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++)
            if (engine_level_preview(level, x, y) == type) n++;
    return n;
}

static void test_levels(void) {
    char label[96];
    const int expected_spawns[5] = {1, 2, 2, 2, 3};
    const int expected_theme[5] = {THEME_MEADOW, THEME_SNOW, THEME_DESERT, THEME_AUTUMN, THEME_VOLCANIC};
    check(engine_level_count() == 5, "five levels");
    for (int lv = 0; lv < engine_level_count(); lv++) {
        snprintf(label, sizeof(label), "level %d loads", lv);
        check(engine_load_level_index(lv) == 1, label);
        snprintf(label, sizeof(label), "level %d is the current level", lv);
        check(engine_current_level() == lv, label);
        snprintf(label, sizeof(label), "level %d has a name and a blurb", lv);
        check(strlen(engine_level_name(lv)) > 0 && strlen(engine_level_blurb(lv)) > 0, label);
        snprintf(label, sizeof(label), "level %d spawn count", lv);
        check(engine_spawn_count() == expected_spawns[lv] && count_type(lv, CELL_SPAWN) == expected_spawns[lv], label);
        snprintf(label, sizeof(label), "level %d has its own theme", lv);
        check(engine_level_theme(lv) == expected_theme[lv], label);
        int features_ok = 1;
        for (int y = 0; y < MAP_H; y++)
            for (int x = 0; x < MAP_W; x++)
                if (engine_level_feature(lv, x, y) && engine_level_preview(lv, x, y) != CELL_BLOCKED) features_ok = 0;
        snprintf(label, sizeof(label), "level %d: every tree, cactus or lava cell is blocked", lv);
        check(features_ok, label);
        snprintf(label, sizeof(label), "level %d has exactly one castle", lv);
        check(count_type(lv, CELL_GOAL) == 1 && engine_goal_cell() >= 0, label);
        int all_reach = 1;
        for (int i = 0; i < engine_spawn_count(); i++) {
            if (engine_distance_to_goal(engine_spawn_at(i)) <= 0) all_reach = 0;
        }
        snprintf(label, sizeof(label), "level %d: every burrow reaches the castle", lv);
        check(all_reach, label);
        snprintf(label, sizeof(label), "level %d: first spawn matches engine_spawn_cell", lv);
        check(engine_spawn_cell() == engine_spawn_at(0), label);
        int same = 1;
        for (int y = 0; y < MAP_H; y++)
            for (int x = 0; x < MAP_W; x++)
                if (engine_level_preview(lv, x, y) != engine_cell_type(x, y)) same = 0;
        snprintf(label, sizeof(label), "level %d: preview matches the loaded grid", lv);
        check(same, label);
    }
    check(engine_spawn_at(-1) == -1 && engine_spawn_at(5) == -1, "spawn index outside the list gives -1");
    check(engine_load_level_index(-1) == 0 && engine_load_level_index(5) == 0, "bad level index refused");
    check(engine_level_preview(0, 16, 0) == CELL_OFFMAP && engine_level_preview(9, 0, 0) == CELL_OFFMAP,
          "preview outside the map is off-map");
    check(strcmp(engine_level_name(7), "") == 0, "name of a missing level is empty");
    check(engine_load_level(NULL) == 1 && engine_current_level() == 0, "engine_load_level still loads the first level");
}

static void test_desert_canyon(void) {
    engine_load_level_index(2);
    check(engine_level_feature(2, 7, 0) == 1 && engine_cell_type(7, 0) == CELL_BLOCKED, "desert canyon: a cactus blocks");
    check(engine_level_feature(2, 10, 0) == 0 && engine_cell_type(10, 0) == CELL_BLOCKED, "desert canyon: a boulder is rock");
    int longest = 0;
    for (int y = 0; y < MAP_H; y++) {
        int run = 0;
        for (int x = 0; x < MAP_W; x++) {
            run = engine_cell_type(x, y) == CELL_BLOCKED ? run + 1 : 0;
            if (run > longest) longest = run;
        }
    }
    for (int x = 0; x < MAP_W; x++) {
        int run = 0;
        for (int y = 0; y < MAP_H; y++) {
            run = engine_cell_type(x, y) == CELL_BLOCKED ? run + 1 : 0;
            if (run > longest) longest = run;
        }
    }
    check(longest <= 3, "desert canyon: rocks are scattered, no straight wall longer than 3 cells");
    check(engine_cell_type(13, 13) == CELL_BUILDABLE, "desert canyon: hidden burrow exit is sand");
    check(engine_can_place_tower(13, 13) == 0, "desert canyon: cannot seal the hidden burrow");
    check(engine_place_tower(13, 13, 1) == 0, "desert canyon: placing there is refused too");
    check(engine_distance_to_goal(engine_spawn_at(0)) >= 14 && engine_distance_to_goal(engine_spawn_at(1)) >= 14,
          "desert canyon: both burrows are at least 14 steps from the castle");
}

static void test_snowy_valley_and_volcanic(void) {
    engine_load_level_index(1);
    int a = engine_spawn_at(0);
    int b = engine_spawn_at(1);
    check(a == 2 * MAP_W + 0 && b == 2 * MAP_W + 15, "snowy valley: burrows on both sides at the top");
    check(engine_goal_cell() == 10 * MAP_W + 8, "snowy valley: castle at the bottom of the valley");
    check(engine_level_feature(1, 0, 0) == 1, "snowy valley: pines in the corner");
    engine_load_level_index(4);
    check(engine_spawn_count() == 3, "volcanic ridge: three burrows");
    int far = 1;
    for (int i = 0; i < engine_spawn_count(); i++) {
        if (engine_distance_to_goal(engine_spawn_at(i)) < 10) far = 0;
    }
    check(far, "volcanic ridge: every burrow is at least 10 steps from the castle");
    check(engine_level_theme(9) == THEME_MEADOW && engine_level_feature(9, 0, 0) == 0 && engine_level_feature(0, 16, 0) == 0,
          "theme and feature of a missing level or cell are safe");
}

static void test_engine_drones(void) {
    int path[MAP_W * MAP_H];
    engine_load_level_index(0);
    int goal = engine_goal_cell();
    int target = 7 * MAP_W + 4;
    int len = engine_drone_path(goal, target, path, MAP_W * MAP_H);
    check(len == 8 && valid_path(path, len, goal, target, MAP_W), "engine drone path from castle to a tower cell");
    int d_before = engine_distance_to_goal(engine_spawn_cell());
    int version_cells = 0;
    for (int x = 5; x <= 10; x++) {
        engine_add_danger(7 * MAP_W + x, 2);
        if (engine_danger(7 * MAP_W + x) == 2) version_cells++;
    }
    check(version_cells == 6, "engine danger added along the straight line");
    len = engine_drone_path(goal, target, path, MAP_W * MAP_H);
    int touches = 0;
    for (int i = 1; i < len - 1; i++) {
        if (engine_danger(path[i]) > 0) touches++;
    }
    check(len > 8 && touches == 0 && valid_path(path, len, goal, target, MAP_W),
          "engine drone path detours round the danger");
    check(engine_distance_to_goal(engine_spawn_cell()) == d_before, "danger does not change the enemy field");
    engine_clear_danger();
    check(engine_danger(7 * MAP_W + 6) == 0, "clear danger empties every cell");
    check(engine_add_danger(-3, 1) == 0 && engine_add_danger(MAP_W * MAP_H, 1) == 0 && engine_add_danger(5, 0) == 0,
          "bad danger calls refused");
    check(engine_drone_path(goal, target, NULL, 10) == 0, "null output refused");
    {
        int walk[MAP_W * MAP_H];
        int spawn = engine_spawn_cell();
        int n = engine_walk_path(spawn, goal, walk, MAP_W * MAP_H);
        check(n == engine_distance_to_goal(spawn) + 1 && walk[0] == spawn && walk[n - 1] == goal,
              "engine walk path from burrow to castle is as short as the enemy route");
        engine_place_tower(1, 4, 7);
        n = engine_walk_path(spawn, goal, walk, MAP_W * MAP_H);
        int blocked = 0;
        for (int i = 0; i < n; i++) {
            if (walk[i] == 4 * MAP_W + 1) blocked = 1;
        }
        check(n > 0 && !blocked, "engine walk path goes round a new tower");
        engine_remove_tower(1, 4);
        check(engine_walk_path(spawn, goal, NULL, 10) == 0 && engine_walk_path(-1, goal, walk, 10) == 0,
              "engine walk path refuses bad input");
    }
    engine_shutdown();
    check(engine_drone_path(0, 1, path, 10) == 0 && engine_spawn_count() == 0, "nothing works after shutdown");
}

int main(void) {
    test_drone_router();
    test_levels();
    test_desert_canyon();
    test_snowy_valley_and_volcanic();
    test_engine_drones();
    printf("\n%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
