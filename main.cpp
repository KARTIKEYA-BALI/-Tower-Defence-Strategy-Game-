#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <cstdio>

#include "game.h"
#include "game_config.h"
#include "gfx.h"

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    if (TTF_Init() != 0) {
        std::fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Siege and Supply", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_W,
                                          WINDOW_H, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        std::fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_RendererInfo info;
    bool vsync = SDL_GetRendererInfo(renderer, &info) == 0 && (info.flags & SDL_RENDERER_PRESENTVSYNC);

    int exitCode = 0;
    {
        Gfx gfx(window, renderer);
        if (!gfx.loadFonts()) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Siege and Supply",
                                     "Could not load the fonts in assets/fonts.\nRun the game from the project folder.",
                                     window);
            exitCode = 1;
        } else {
            Game game(gfx);
            Uint32 lastTicks = SDL_GetTicks();
            while (game.isRunning()) {
                Uint32 frameStart = SDL_GetTicks();
                SDL_Event e;
                while (SDL_PollEvent(&e)) game.handleEvent(e);

                Uint32 now = SDL_GetTicks();
                float dt = (now - lastTicks) / 1000.0f;
                lastTicks = now;

                game.update(dt);
                game.draw();

                Uint32 spent = SDL_GetTicks() - frameStart;
                if (!vsync && spent < 16) SDL_Delay(16 - spent);
            }
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return exitCode;
}
