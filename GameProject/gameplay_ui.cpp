
#include "gameplay_ui.h"

void drawGameMap(SDL_Renderer* renderer)
{
    // 1. Draw green grass background
    SDL_SetRenderDrawColor(renderer, 80, 150, 70, 255);
    SDL_Rect grass = {0, 0, 1000, 700};
    SDL_RenderFillRect(renderer, &grass);

    // 2. Draw grid lines
    SDL_SetRenderDrawColor(renderer, 55, 110, 50, 255);

    for (int x = 0; x <= 1000; x += 50)
    {
        SDL_RenderDrawLine(renderer, x, 0, x, 700);
    }

    for (int y = 0; y <= 700; y += 50)
    {
        SDL_RenderDrawLine(renderer, 0, y, 1000, y);
    }

    // 3. Draw the winding dirt road
    SDL_SetRenderDrawColor(renderer, 160, 120, 75, 255);

    SDL_Rect road1 = {0, 300, 350, 100};
    SDL_RenderFillRect(renderer, &road1);

    SDL_Rect road2 = {300, 300, 100, 250};
    SDL_RenderFillRect(renderer, &road2);

    SDL_Rect road3 = {300, 450, 400, 100};
    SDL_RenderFillRect(renderer, &road3);

    // 4. Draw the castle/base
    SDL_SetRenderDrawColor(renderer, 180, 180, 190, 255);

    SDL_Rect castle = {650, 450, 100, 100};
    SDL_RenderFillRect(renderer, &castle);

    // 5. Draw a darker outline around the castle
    SDL_SetRenderDrawColor(renderer, 50, 50, 60, 255);
    SDL_RenderDrawRect(renderer, &castle);
}
