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

    float scrollY;
    float scrollTargetY;

    SDL_bool running;
} TerneditState;

void TerneditInit(TerneditState* te);
void TerneditFree(TerneditState* te);

void _TerneditHandle(TerneditState* te, const SDL_Event* event);
void _TerneditUpdate(TerneditState* te, float dt);
void _TerneditDraw(TerneditState* te);

int TerneditRun(TerneditState* te);
