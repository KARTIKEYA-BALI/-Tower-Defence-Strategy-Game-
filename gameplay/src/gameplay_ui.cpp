#include "../include/gameplay_ui.h"

void drawGameMap(SDL_Renderer* renderer)
{
    SDL_Rect mapArea = {50, 50, 900, 600};


    SDL_SetRenderDrawColor(renderer, 25, 28, 35, 255);
    SDL_RenderFillRect(renderer, &mapArea);

  
    SDL_SetRenderDrawColor(renderer, 45, 50, 60, 255);

    const int cellSize = 50;

    for (int x = mapArea.x; x <= mapArea.x + mapArea.w; x += cellSize)
    {
        SDL_RenderDrawLine(
            renderer,
            x,
            mapArea.y,
            x,
            mapArea.y + mapArea.h
        );
    }

    for (int y = mapArea.y; y <= mapArea.y + mapArea.h; y += cellSize)
    {
        SDL_RenderDrawLine(
            renderer,
            mapArea.x,
            y,
            mapArea.x + mapArea.w,
            y
        );
    }


    SDL_SetRenderDrawColor(renderer, 120, 90, 60, 255);

    SDL_Rect path1 = {50, 250, 300, 50};
    SDL_Rect path2 = {300, 250, 50, 200};
    SDL_Rect path3 = {300, 400, 400, 50};
    SDL_Rect path4 = {650, 150, 50, 300};
    SDL_Rect path5 = {650, 150, 300, 50};

    SDL_RenderFillRect(renderer, &path1);
    SDL_RenderFillRect(renderer, &path2);
    SDL_RenderFillRect(renderer, &path3);
    SDL_RenderFillRect(renderer, &path4);
    SDL_RenderFillRect(renderer, &path5);


    SDL_SetRenderDrawColor(renderer, 160, 50, 50, 255);

    SDL_Rect base = {900, 100, 50, 100};

    SDL_RenderFillRect(renderer, &base);
}


void drawTowers(SDL_Renderer* renderer)
{
    SDL_SetRenderDrawColor(renderer, 60, 120, 220, 255);

    SDL_Rect tower1 = {150, 150, 40, 40};
    SDL_Rect tower2 = {500, 100, 40, 40};
    SDL_Rect tower3 = {750, 500, 40, 40};

    SDL_RenderFillRect(renderer, &tower1);
    SDL_RenderFillRect(renderer, &tower2);
    SDL_RenderFillRect(renderer, &tower3);
}


void drawEnemies(SDL_Renderer* renderer)
{
    SDL_SetRenderDrawColor(renderer, 200, 60, 60, 255);

    SDL_Rect enemy1 = {100, 255, 30, 30};
    SDL_Rect enemy2 = {200, 255, 30, 30};
    SDL_Rect enemy3 = {350, 420, 30, 30};

    SDL_RenderFillRect(renderer, &enemy1);
    SDL_RenderFillRect(renderer, &enemy2);
    SDL_RenderFillRect(renderer, &enemy3);


    SDL_SetRenderDrawColor(renderer, 60, 200, 80, 255);

    SDL_Rect health1 = {100, 248, 30, 5};
    SDL_Rect health2 = {200, 248, 30, 5};
    SDL_Rect health3 = {350, 413, 30, 5};

    SDL_RenderFillRect(renderer, &health1);
    SDL_RenderFillRect(renderer, &health2);
    SDL_RenderFillRect(renderer, &health3);
}


void drawProjectiles(SDL_Renderer* renderer)
{
    SDL_SetRenderDrawColor(renderer, 255, 220, 80, 255);

    SDL_Rect projectile1 = {180, 210, 10, 10};
    SDL_Rect projectile2 = {530, 140, 10, 10};
    SDL_Rect projectile3 = {780, 450, 10, 10};

    SDL_RenderFillRect(renderer, &projectile1);
    SDL_RenderFillRect(renderer, &projectile2);
    SDL_RenderFillRect(renderer, &projectile3);
}


void drawDrones(SDL_Renderer* renderer)
{
    SDL_SetRenderDrawColor(renderer, 80, 220, 220, 255);

    SDL_Rect drone1 = {250, 180, 25, 25};
    SDL_Rect drone2 = {600, 350, 25, 25};

    SDL_RenderFillRect(renderer, &drone1);
    SDL_RenderFillRect(renderer, &drone2);
}


void drawWaveInfo(SDL_Renderer* renderer)
{
  
    SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);

    SDL_Rect waveBar = {1000, 60, 220, 25};
    SDL_RenderFillRect(renderer, &waveBar);

   
    SDL_SetRenderDrawColor(renderer, 80, 180, 80, 255);

    SDL_Rect progress = {1000, 60, 140, 25};
    SDL_RenderFillRect(renderer, &progress);
}


void drawGameStats(SDL_Renderer* renderer)
{
    // Health
    SDL_SetRenderDrawColor(renderer, 180, 50, 50, 255);

    SDL_Rect healthBar = {1000, 110, 220, 20};
    SDL_RenderFillRect(renderer, &healthBar);

    // Resources
    SDL_SetRenderDrawColor(renderer, 220, 180, 60, 255);

    SDL_Rect resourceBar = {1000, 150, 220, 20};
    SDL_RenderFillRect(renderer, &resourceBar);
}