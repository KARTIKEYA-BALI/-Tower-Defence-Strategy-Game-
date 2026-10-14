#ifndef UI_BATTLEFIELD_H
#define UI_BATTLEFIELD_H

#include <SDL2/SDL.h>

void drawGameMap1(SDL_Renderer* renderer);
void drawGameMap2(SDL_Renderer* renderer);
void drawTowers(SDL_Renderer* renderer);
void drawEnemies(SDL_Renderer* renderer);
void drawProjectiles(SDL_Renderer* renderer);
void drawDrones(SDL_Renderer* renderer);
void drawWaveInfo(SDL_Renderer* renderer);
void drawGameStats(SDL_Renderer* renderer);

#endif