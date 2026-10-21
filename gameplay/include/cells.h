#ifndef CELLS_H
#define CELLS_H

#include "game_config.h"

inline int cellIndex(int cx, int cy) {
    return cy * MAP_W + cx;
}

inline int cellX(int cell) {
    return cell % MAP_W;
}

inline int cellY(int cell) {
    return cell / MAP_W;
}

inline float cellCentreX(int cell) {
    return cellX(cell) * CELL_SIZE + CELL_SIZE * 0.5f;
}

inline float cellCentreY(int cell) {
    return cellY(cell) * CELL_SIZE + CELL_SIZE * 0.5f;
}

inline bool insideMap(int cx, int cy) {
    return cx >= 0 && cx < MAP_W && cy >= 0 && cy < MAP_H;
}

inline bool pointOnMap(int px, int py) {
    return px >= 0 && px < PANEL_X && py >= 0 && py < MAP_H * CELL_SIZE;
}

#endif
