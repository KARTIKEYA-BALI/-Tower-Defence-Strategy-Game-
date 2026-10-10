
#ifndef CONTROL_UI_H
#define CONTROL_UI_H

#include <SDL.h>
#include <SDL_ttf.h>

void drawControlUI(SDL_Renderer* renderer,
                   TTF_Font* font,
                   TTF_Font* titleFont,
                   int page,
                   int mouseX,
                   int mouseY);

int handleControlUIClick(int x, int y, int page);

#endif
