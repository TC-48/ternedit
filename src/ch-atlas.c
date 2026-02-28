#include <ch-atlas.h>
#include <stddef.h>

CharTextureAtlas LoadCharTextures(TTF_Font* regular, TTF_Font* italic, SDL_Renderer* renderer) {
    CharTextureAtlas atlas = { 0 };

    SDL_Color white = { 255, 255, 255, 255 };
    const char symbols[] = { '0', '1', '2' };
   
    TTF_Font* fonts[] = { regular, italic };

    for (size_t type = 0; type < 2; ++type) {
        for (size_t i = 0; i < 3; ++i) {
            char txt[2] = { symbols[i], 0 };
    
            SDL_Surface* surf = TTF_RenderUTF8_Blended(fonts[type], txt, white);
            if (!surf) return atlas;

            SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
            if (!tex) return atlas;

            atlas.chars[type][i] = tex;
    
            if (atlas.charWidth < surf->w)  atlas.charWidth  = surf->w;
            if (atlas.charHeight < surf->h) atlas.charHeight = surf->h;

            SDL_FreeSurface(surf);
        }
    }

    atlas.isLoaded = SDL_TRUE;
    return atlas;
}

void FreeCharTextures(CharTextureAtlas* atlas) {
    for (size_t type = 0; type < 2; ++type) {
        for (size_t i = 0; i < 3; ++i) {
            if (atlas->chars[type][i] != NULL) {
                SDL_DestroyTexture(atlas->chars[type][i]);
                atlas->chars[type][i] = NULL;
            }
        }
    }
}
