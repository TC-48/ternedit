#pragma once

#include <SDL2/SDL_render.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_ttf.h>

#include <stddef.h>
#include <tritstate.h>

#include <ch-atlas.h>

#define DEFAULT_WIN_WIDTH 680
#define DEFAULT_WIN_HEIGHT 480

#define MIN_WIN_WIDTH 200
#define MIN_WIN_HEIGHT 150

#define DEFAULT_FONT_SIZE 88

typedef struct TerneditState {
    TritState buf[1024];
    size_t bufSize;
    size_t cursorPos;

    SDL_Window* win;
    SDL_Renderer* renderer;

    CharTextureAtlas atlas;

    struct {
        TTF_Font* regular;
        TTF_Font* italic;
    } editorFont;

    int winW, winH;
    int groupWidth;
    int groups;
    size_t charsPerRow;

    SDL_bool running;
} TerneditState;

void ternedit_init(TerneditState* te);
void ternedit_free(TerneditState* te);

void _ternedit_handle(TerneditState* te, const SDL_Event* event);
void _ternedit_update(TerneditState* te, float dt);
void _ternedit_draw(TerneditState* te);

int ternedit_run(TerneditState* te);
