#include <SDL2/SDL.h>
#include "gameplay/include/gameplay_ui.h"
int main()

{if (SDL_Init(SDL_INIT_VIDEO) != 0)
{
    return 1;
}
SDL_Window* window = SDL_CreateWindow(
    "Siege and Supply",
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    1280,
    720,
    SDL_WINDOW_SHOWN
);
SDL_Renderer* renderer = SDL_CreateRenderer(
    window,
    -1,
    SDL_RENDERER_ACCELERATED
);
if (renderer == nullptr)
{
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
}
 
    bool running = true;

while (running)
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            running = false;
        }
    }
     SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);

    SDL_RenderClear(renderer);

drawGameMap(renderer);
drawTowers(renderer);
drawEnemies(renderer);
drawProjectiles(renderer);
drawDrones(renderer);


drawWaveInfo(renderer);

drawGameStats(renderer);
SDL_RenderPresent(renderer);
SDL_Delay(16);
}
}
