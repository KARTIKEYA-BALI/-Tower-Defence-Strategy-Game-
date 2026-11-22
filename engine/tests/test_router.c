#include <stdio.h>
#include <stdlib.h>
#include "router.h"

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

static int walk(const Grid *g, const int dist[], int start, int goal, int avoid) {
    int c = start;
    int steps = 0;
    while (c != goal && steps < 1000) {
        int n = route_next_step(g, dist, c);
        if (n == c || n == avoid) return -1;
        c = n;
        steps++;
    }
    return c == goal ? steps : -1;
}

static void column_wall(Grid *g, int x, int gap_y) {
    for (int y = 0; y < g->height; y++) {
        if (y != gap_y) grid_set_terrain(g, x, y, CELL_BLOCKED);
    }
}

static void load_game_level(Grid *g) {
    const char *level[14] = {
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
    grid_init(g, 16, 14);
    for (int y = 0; y < 14; y++) {
        for (int x = 0; x < 16; x++) {
            char ch = level[y][x];
            if (ch == '#') grid_set_terrain(g, x, y, CELL_BLOCKED);
            else if (ch == '=') grid_set_terrain(g, x, y, CELL_PATH);
            else if (ch == 'S') grid_set_terrain(g, x, y, CELL_SPAWN);
            else if (ch == 'G') grid_set_terrain(g, x, y, CELL_GOAL);
        }
    }
}

static void print_field(const Grid *g, const int dist[]) {
    for (int y = 0; y < g->height; y++) {
        for (int x = 0; x < g->width; x++) {
            int i = grid_index(g, x, y);
            CellType t = grid_get_terrain(g, x, y);
            if (t == CELL_BLOCKED) printf(" ##");
            else if (g->cells[i].tower_id != -1) printf(" TT");
            else if (t == CELL_SPAWN) printf("  S");
            else if (t == CELL_GOAL) printf("  G");
            else if (dist[i] == ROUTE_UNREACHABLE) printf(" --");
            else printf("%3d", dist[i]);
        }
        printf("\n");
    }
}

int main(void) {
    Grid g;
    int dist[16 * 14];
    int s, t, v, ok;

    printf("--- Map A: open 5x5, spawn (0,0), goal (4,4)\n");
    grid_init(&g, 5, 5);
    s = grid_index(&g, 0, 0);
    t = grid_index(&g, 4, 4);
    check(route_path_exists(&g, s, t) == 1, "A: path exists on an open map");
    check(route_path_exists(&g, t, t) == 1, "A: path exists when start == goal");
    check(route_build_distance_field(&g, t, dist) == 1, "A: distance field builds");
    check(dist[t] == 0, "A: dist[goal] == 0");
    check(dist[s] == 8, "A: dist[spawn] == 8");
    check(walk(&g, dist, s, t, -1) == 8, "A: following next_step reaches the goal in 8 steps");
    grid_free(&g);

    printf("--- Map B: 5x5 wall at x = 2 with one gap at (2,0), spawn (0,2), goal (4,2)\n");
    grid_init(&g, 5, 5);
    column_wall(&g, 2, 0);
    s = grid_index(&g, 0, 2);
    t = grid_index(&g, 4, 2);
    check(route_path_exists(&g, s, t) == 1, "B: path exists through the gap");
    route_build_distance_field(&g, t, dist);
    check(dist[s] == 8, "B: dist[spawn] == 8 (up to the gap and back down)");
    check(dist[grid_index(&g, 2, 3)] == ROUTE_UNREACHABLE, "B: a rock cell has no distance");
    check(walk(&g, dist, s, t, -1) == 8, "B: walk takes 8 steps");
    v = g.version;
    check(route_can_place(&g, 2, 0, s, t) == 0, "B: can_place on the gap is refused (would seal the path)");
    check(route_can_place(&g, 4, 4, s, t) == 1, "B: can_place on an open cell is allowed");
    check(g.cells[grid_index(&g, 2, 0)].tower_id == -1 && g.cells[grid_index(&g, 4, 4)].tower_id == -1,
          "B: can_place leaves no pretend tower behind");
    check(g.version == v, "B: can_place does not change the version");
    check(route_can_place(&g, 2, 3, s, t) == 0, "B: can_place on a rock is refused");
    check(route_can_place(&g, 0, 2, s, t) == 0, "B: can_place on the spawn is refused");
    check(route_can_place(&g, 4, 2, s, t) == 0, "B: can_place on the goal is refused");
    check(route_can_place(&g, 5, 0, s, t) == 0, "B: can_place off the map is refused");
    grid_place_tower(&g, 4, 4, 7);
    check(route_can_place(&g, 4, 4, s, t) == 0, "B: can_place on a cell with a tower is refused");
    check(g.cells[grid_index(&g, 4, 4)].tower_id == 7, "B: the real tower is still there afterwards");
    grid_free(&g);

    printf("--- Map C: 5x5 fully sealed at x = 2, spawn (0,2), goal (4,2)\n");
    grid_init(&g, 5, 5);
    column_wall(&g, 2, -1);
    s = grid_index(&g, 0, 2);
    t = grid_index(&g, 4, 2);
    check(route_path_exists(&g, s, t) == 0, "C: no path through a sealed wall");
    check(route_build_distance_field(&g, t, dist) == 1, "C: distance field still builds");
    check(dist[s] == ROUTE_UNREACHABLE, "C: dist[spawn] == -1");
    check(route_next_step(&g, dist, s) == s, "C: next_step from the spawn waits (returns the same cell)");
    grid_free(&g);

    printf("--- Re-routing: open 5x5, spawn (0,2), goal (4,2)\n");
    grid_init(&g, 5, 5);
    s = grid_index(&g, 0, 2);
    t = grid_index(&g, 4, 2);
    route_build_distance_field(&g, t, dist);
    check(dist[s] == 4, "R: straight line, dist[spawn] == 4");
    v = g.version;
    check(route_can_place(&g, 2, 2, s, t) == 1, "R: a tower in the middle of the line is allowed");
    grid_place_tower(&g, 2, 2, 0);
    check(g.version != v, "R: placing the tower changes the version");
    route_build_distance_field(&g, t, dist);
    check(dist[s] == 6, "R: after the tower, dist[spawn] == 6 (round the tower)");
    check(walk(&g, dist, s, t, grid_index(&g, 2, 2)) == 6, "R: the new walk takes 6 steps and never enters the tower");
    grid_remove_tower(&g, 2, 2);
    route_build_distance_field(&g, t, dist);
    check(dist[s] == 4, "R: after selling the tower, back to 4");
    grid_free(&g);

    printf("--- Bad input\n");
    grid_init(&g, 5, 5);
    route_build_distance_field(&g, 0, dist);
    check(route_next_step(&g, dist, -1) == -1, "next_step(-1) == -1");
    check(route_next_step(&g, dist, 25) == -1, "next_step(width*height) == -1");
    check(route_next_step(&g, dist, 0) == 0, "next_step at the goal stays at the goal");
    check(route_build_distance_field(&g, 25, dist) == 0, "distance field with goal off the map returns 0");
    ok = 1;
    for (int i = 0; i < 25; i++) if (dist[i] != ROUTE_UNREACHABLE) ok = 0;
    check(ok, "a failed distance field leaves every cell at -1");
    grid_set_terrain(&g, 1, 1, CELL_BLOCKED);
    check(route_build_distance_field(&g, grid_index(&g, 1, 1), dist) == 0, "distance field with goal on a rock returns 0");
    check(route_path_exists(&g, grid_index(&g, 1, 1), 0) == 0, "path_exists with a blocked start returns 0");
    check(route_path_exists(&g, 0, grid_index(&g, 1, 1)) == 0, "path_exists with a blocked goal returns 0");
    check(route_path_exists(&g, -1, 0) == 0 && route_path_exists(&g, 0, 25) == 0, "path_exists with cells off the map returns 0");
    grid_free(&g);

    printf("--- Random maps: BFS and Dijkstra must agree\n");
    srand(42);
    ok = 1;
    for (int trial = 0; trial < 500; trial++) {
        int w = 2 + rand() % 15;
        int h = 2 + rand() % 13;
        grid_init(&g, w, h);
        for (int i = 0; i < w * h; i++) {
            int r = rand() % 10;
            if (r < 3) g.cells[i].terrain = CELL_BLOCKED;
            else if (r == 3) g.cells[i].tower_id = 1;
        }
        int a = rand() % (w * h);
        int b = rand() % (w * h);
        int built = route_build_distance_field(&g, b, dist);
        if (built) {
            int bfs = route_path_exists(&g, a, b);
            if (bfs != (dist[a] != ROUTE_UNREACHABLE)) ok = 0;
            for (int i = 0; i < w * h; i++) {
                if (dist[i] > 0) {
                    int n = route_next_step(&g, dist, i);
                    if (n == i || dist[n] != dist[i] - 1) ok = 0;
                }
            }
        }
        grid_free(&g);
    }
    check(ok, "500 random maps: path_exists agrees with the distance field, and every next_step is one step closer");

    printf("--- The game map (16 x 14)\n");
    load_game_level(&g);
    s = grid_index(&g, 0, 4);
    t = grid_index(&g, 11, 7);
    check(route_path_exists(&g, s, t) == 1, "game map: path exists");
    check(route_build_distance_field(&g, t, dist) == 1, "game map: distance field builds");
    check(dist[s] == 14, "game map: spawn is 14 steps from the castle (same as the frontend mock)");
    check(walk(&g, dist, s, t, -1) == 14, "game map: following next_step takes 14 steps");

    printf("\nDistance field before towers (numbers = steps to the castle):\n");
    print_field(&g, dist);
    int before = dist[s];

    int placed = 0;
    for (int y = 2; y <= 8; y++) {
        if (route_can_place(&g, 5, y, s, t)) {
            grid_place_tower(&g, 5, y, 100 + y);
            placed++;
        }
    }
    route_build_distance_field(&g, t, dist);
    printf("\nDistance field after %d towers at x = 5, rows 2 to 8:\n", placed);
    print_field(&g, dist);
    printf("\n");
    check(placed == 7, "game map: all 7 towers of the short wall are allowed");
    check(dist[s] > before, "game map: the route got longer after the towers");
    check(walk(&g, dist, s, t, -1) == dist[s], "game map: enemies follow the new route to the castle");

    int refused = 0;
    int refused_y = -1;
    for (int y = 0; y < 14; y++) {
        if (route_can_place(&g, 5, y, s, t)) grid_place_tower(&g, 5, y, 200 + y);
        else if (g.cells[grid_index(&g, 5, y)].tower_id == -1) {
            refused++;
            refused_y = y;
        }
    }
    printf("Closing the whole column x = 5: refused at row %d\n", refused_y);
    check(refused == 1, "game map: closing the last gap of a full wall is refused");
    check(route_path_exists(&g, s, t) == 1, "game map: the path still exists after the refusal");
    grid_free(&g);

    {
        Grid w;
        int path[64];
        int len;
        grid_init(&w, 5, 5);
        len = route_walk_path(&w, 0, 24, path, 64);
        check(len == 9 && path[0] == 0 && path[8] == 24, "walk path: open 5x5 corner to corner is 9 cells");
        int steps_ok = 1;
        for (int i = 1; i < len; i++) {
            int dx = path[i] % 5 - path[i - 1] % 5;
            int dy = path[i] / 5 - path[i - 1] / 5;
            if (dx * dx + dy * dy != 1) steps_ok = 0;
        }
        check(steps_ok, "walk path: every step goes to a neighbour");
        check(route_walk_path(&w, 12, 12, path, 64) == 1 && path[0] == 12, "walk path: start equals goal");
        for (int y = 0; y < 4; y++) grid_place_tower(&w, 2, y, 50 + y);
        len = route_walk_path(&w, 0, 4, path, 64);
        check(len == 13, "walk path: goes round a tower wall through the only gap");
        int touches_tower = 0;
        for (int i = 0; i < len; i++) {
            if (w.cells[path[i]].tower_id >= 0) touches_tower = 1;
        }
        check(!touches_tower, "walk path: never steps on a tower");
        grid_place_tower(&w, 2, 4, 54);
        check(route_walk_path(&w, 0, 4, path, 64) == 0, "walk path: sealed off gives 0");
        check(route_walk_path(&w, 0, 2 * 5 + 2, path, 64) == 0, "walk path: a tower as the goal gives 0");
        grid_remove_tower(&w, 2, 4);
        grid_place_tower(&w, 0, 0, 60);
        len = route_walk_path(&w, 0, 1, path, 64);
        check(len == 2 && path[0] == 0 && path[1] == 1, "walk path: can leave a start cell that has a tower");
        check(route_walk_path(&w, 0, 24, path, 3) == 0, "walk path: too long for the buffer gives 0");
        check(route_walk_path(&w, -1, 3, path, 64) == 0 && route_walk_path(&w, 0, 25, path, 64) == 0 &&
                  route_walk_path(NULL, 0, 3, path, 64) == 0 && route_walk_path(&w, 0, 3, NULL, 64) == 0,
              "walk path: bad input refused");
        grid_free(&w);
    }

    printf("\n%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
