#pragma once

#include <SDL2/SDL_render.h>
#include <SDL2/SDL_ttf.h>

#define REGULAR 0
#define ITALIC 1

typedef struct CharTextureAtlas {
    SDL_Texture* chars[2][3];
    int charWidth, charHeight;
    SDL_bool isLoaded;
} CharTextureAtlas;

CharTextureAtlas LoadCharTextures(TTF_Font* regular, TTF_Font* italic, SDL_Renderer* renderer);
void             FreeCharTextures(CharTextureAtlas* atlas);
