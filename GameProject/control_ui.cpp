
#include "control_ui.h"
#include <cstdio>

void drawText(SDL_Renderer* renderer, TTF_Font* font,
              const char* text, int x, int y, SDL_Color color)
{
    if (font == NULL)
    {
        printf("Font is NULL!\n");
        return;
    }

    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text, color);

    if (surface == NULL)
    {
        printf("Text surface error: %s\n", TTF_GetError());
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

    if (texture == NULL)
    {
        printf("Text texture error: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect position = {x, y, surface->w, surface->h};

    SDL_FreeSurface(surface);

    SDL_RenderCopy(renderer, texture, NULL, &position);
    SDL_DestroyTexture(texture);
}

void drawButton(SDL_Renderer* renderer, TTF_Font* font,
                const char* text, SDL_Rect button,
                int mouseX, int mouseY,
                SDL_Color normal, SDL_Color hover)
{
    SDL_Color color = normal;

    if (mouseX >= button.x && mouseX < button.x + button.w &&
        mouseY >= button.y && mouseY < button.y + button.h)
    {
        color = hover;
    }

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
    SDL_RenderFillRect(renderer, &button);

    SDL_SetRenderDrawColor(renderer, 235, 235, 245, 255);
    SDL_RenderDrawRect(renderer, &button);

    SDL_Color white = {255, 255, 255, 255};

    int textWidth = 0;
    int textHeight = 0;

    if (TTF_SizeUTF8(font, text, &textWidth, &textHeight) == 0)
    {
        drawText(renderer, font, text,
                 button.x + (button.w - textWidth) / 2,
                 button.y + (button.h - textHeight) / 2,
                 white);
    }
}

void drawControlUI(SDL_Renderer* renderer, TTF_Font* font,
                   TTF_Font* titleFont, int page,
                   int mouseX, int mouseY)
{
    SDL_SetRenderDrawColor(renderer, 20, 28, 45, 255);
    SDL_RenderClear(renderer);

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color gold = {255, 205, 105, 255};

    if (page == 0)
    {
        SDL_Rect titlePanel = {200, 50, 600, 100};

        SDL_SetRenderDrawColor(renderer, 45, 65, 95, 255);
        SDL_RenderFillRect(renderer, &titlePanel);

        SDL_SetRenderDrawColor(renderer, 235, 235, 245, 255);
        SDL_RenderDrawRect(renderer, &titlePanel);

        int width = 0;
        int height = 0;

        if (TTF_SizeUTF8(titleFont, "SIEGE AND SUPPLY",
                         &width, &height) == 0)
        {
            drawText(renderer, titleFont, "SIEGE AND SUPPLY",
                     (1000 - width) / 2, 75, gold);
        }

        drawButton(renderer, font, "START GAME",
                   {350, 220, 300, 65}, mouseX, mouseY,
                   {45, 125, 85, 255}, {65, 175, 110, 255});

        drawButton(renderer, font, "SETTINGS",
                   {350, 320, 300, 65}, mouseX, mouseY,
                   {65, 85, 145, 255}, {90, 120, 195, 255});

        drawButton(renderer, font, "EXIT",
                   {350, 420, 300, 65}, mouseX, mouseY,
                   {145, 55, 60, 255}, {195, 75, 80, 255});
    }
    else if (page == 1)
    {
        drawText(renderer, titleFont, "BATTLEFIELD",
                 350, 100, gold);

        drawText(renderer, font, "Gameplay screen placeholder",
                 350, 250, white);

        drawText(renderer, font, "Your game grid will go here.",
                 340, 300, white);

        drawButton(renderer, font, "BACK TO MENU",
                   {350, 550, 300, 60}, mouseX, mouseY,
                   {65, 85, 145, 255}, {90, 120, 195, 255});
    }
    else if (page == 2)
    {
        drawText(renderer, titleFont, "SETTINGS",
                 390, 120, gold);

        drawText(renderer, font, "Settings screen",
                 405, 250, white);

        drawText(renderer, font, "More options can be added later.",
                 325, 300, white);

        drawButton(renderer, font, "BACK TO MENU",
                   {350, 550, 300, 60}, mouseX, mouseY,
                   {65, 85, 145, 255}, {90, 120, 195, 255});
    }
}

int handleControlUIClick(int x, int y, int page)
{
    if (page == 0)
    {
        if (x >= 350 && x < 650 && y >= 220 && y < 285)
            return 1;

        if (x >= 350 && x < 650 && y >= 320 && y < 385)
            return 2;

        if (x >= 350 && x < 650 && y >= 420 && y < 485)
            return 3;
    }
    else
    {
        if (x >= 350 && x < 650 && y >= 550 && y < 610)
            return 0;
    }

    return page;
}
