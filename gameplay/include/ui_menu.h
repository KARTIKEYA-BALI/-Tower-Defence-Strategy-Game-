
#ifndef UI_MENU_H
#define UI_MENU_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

void drawControlUI(SDL_Renderer* renderer,
                   TTF_Font* font,
                   TTF_Font* titleFont,
                   int page,
                   int mouseX,
                   int mouseY);

int handleControlUIClick(int x, int y, int page);

#endif
