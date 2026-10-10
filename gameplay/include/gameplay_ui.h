#ifndef GAMEPLAY_UI_H
#define GAMEPLAY_UI_H

#include <SDL2/SDL.h>

void drawGameMap(SDL_Renderer* renderer);
void drawTowers(SDL_Renderer* renderer);
void drawEnemies(SDL_Renderer* renderer);
void drawProjectiles(SDL_Renderer* renderer);
void drawDrones(SDL_Renderer* renderer);
void drawWaveInfo(SDL_Renderer* renderer);
void drawGameStats(SDL_Renderer* renderer);

#endif