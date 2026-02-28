#include <SDL2/SDL_render.h>

#define DEFAULT 0
#define ITALIC 1

typedef struct CharTextureAtlas {
    SDL_Texture* chars[2][3];
} CharTextureAtlas;
