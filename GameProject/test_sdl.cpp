
#include <SDL.h>
#include <iostream>
#include "gameplay_ui.h"

using namespace std;

const int WINDOW_WIDTH = 1000;
const int WINDOW_HEIGHT = 700;

void drawButton(SDL_Renderer* renderer,
                int x, int y, int w, int h,
                Uint8 r, Uint8 g, Uint8 b)
{
    SDL_Rect button = {x, y, w, h};

    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    SDL_RenderFillRect(renderer, &button);

    SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
    SDL_RenderDrawRect(renderer, &button);
}

int main(int argc, char* argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        cout << "SDL initialization failed: "
             << SDL_GetError() << endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Siege and Supply",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (window == nullptr)
    {
        cout << "Window creation failed: "
             << SDL_GetError() << endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED
    );

    if (renderer == nullptr)
    {
        cout << "Renderer creation failed: "
             << SDL_GetError() << endl;

        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool running = true;
    SDL_Event event;

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
        }

        // Clear the window
        SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
        SDL_RenderClear(renderer);

        // Draw battlefield
        drawGameMap(renderer);

        // Draw right-side control panel background
        SDL_SetRenderDrawColor(renderer, 35, 40, 55, 255);
        SDL_Rect panel = {800, 0, 200, 700};
        SDL_RenderFillRect(renderer, &panel);

        // Draw named button areas
        drawButton(renderer, 820, 50, 160, 55, 50, 100, 180);
        drawButton(renderer, 820, 130, 160, 55, 50, 150, 100);
        drawButton(renderer, 820, 210, 160, 55, 180, 120, 50);
        drawButton(renderer, 820, 290, 160, 55, 150, 80, 80);
        drawButton(renderer, 820, 370, 160, 55, 100, 90, 170);

        // Draw a title bar
        SDL_SetRenderDrawColor(renderer, 25, 30, 45, 255);
        SDL_Rect title = {0, 0, 800, 45};
        SDL_RenderFillRect(renderer, &title);

        // Draw a simple base health bar
        SDL_SetRenderDrawColor(renderer, 180, 40, 40, 255);
        SDL_Rect healthBackground = {30, 60, 200, 20};
        SDL_RenderFillRect(renderer, &healthBackground);

        SDL_SetRenderDrawColor(renderer, 50, 200, 80, 255);
        SDL_Rect health = {30, 60, 160, 20};
        SDL_RenderFillRect(renderer, &health);

        // Present the completed frame
        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
