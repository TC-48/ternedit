#include <ternedit.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>
#include <time.h>

void ternedit_init(TerneditState* te) {
    te->bufSize = 243;
    te->cursorPos = 0;
    te->running = SDL_TRUE;

    for (size_t i = 0; i < te->bufSize; ++i) {
        te->buf[i] = rand() % 3;
    }

    // TODO: this probably should be moved out of this function
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    te->win = SDL_CreateWindow("ternedit", 0, 0, DEFAULT_WIN_WIDTH, DEFAULT_WIN_HEIGHT, SDL_WINDOW_RESIZABLE);
    te->renderer = SDL_CreateRenderer(te->win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    te->editorFont.regular = TTF_OpenFont("assets/fonts/monogram/ttf/monogram.ttf", DEFAULT_FONT_SIZE);
    te->editorFont.italic = TTF_OpenFont("assets/fonts/monogram/ttf/monogram-extended-italic.ttf", DEFAULT_FONT_SIZE);

    te->atlas = LoadCharTextures(te->editorFont.regular, te->editorFont.italic, te->renderer);

    SDL_GetWindowSize(te->win, &te->winW, &te->winH);
    te->groupWidth = 6 * te->atlas.charWidth + 17;
    te->groups = te->winW / te->groupWidth;
    if (te->groups < 1) te->groups = 1;
    te->charsPerRow = (size_t) (te->groups * 6);
}

void ternedit_free(TerneditState* te) {
    FreeCharTextures(&te->atlas);
    TTF_CloseFont(te->editorFont.regular);
    TTF_CloseFont(te->editorFont.italic);
    SDL_DestroyRenderer(te->renderer);
    SDL_DestroyWindow(te->win);

    // TODO: this probably should be moved out of this function
    TTF_Quit();
    SDL_Quit();
}

void _ternedit_handle(TerneditState* te, const SDL_Event* event) {
    switch (event->type) {
    case SDL_QUIT:
        te->running = SDL_FALSE;
        break;
    case SDL_KEYUP:
        switch (event->key.keysym.scancode) {
        case SDL_SCANCODE_LEFT:
            if (te->cursorPos > 0) te->cursorPos--;
            break;
        case SDL_SCANCODE_RIGHT:
            if (te->cursorPos + 1 < te->bufSize) te->cursorPos++;
            break;
        case SDL_SCANCODE_UP:
            if (te->cursorPos >= te->charsPerRow) {
                te->cursorPos -= te->charsPerRow;
            }
            break;
        case SDL_SCANCODE_DOWN:
            if (te->cursorPos + te->charsPerRow < te->bufSize) {
                te->cursorPos += te->charsPerRow;
            }
            break;
        case SDL_SCANCODE_0:
            te->buf[te->cursorPos] = 0;
            if (te->cursorPos + 1 < te->bufSize) te->cursorPos++;
            break;
        case SDL_SCANCODE_1:
            te->buf[te->cursorPos] = 1;
            if (te->cursorPos + 1 < te->bufSize) te->cursorPos++;
            break;
        case SDL_SCANCODE_2:
            te->buf[te->cursorPos] = 2;
            if (te->cursorPos + 1 < te->bufSize) te->cursorPos++;
            break;
        case SDL_SCANCODE_F:
            te->buf[te->cursorPos] = (te->buf[te->cursorPos] + 1) % 3;
            break;
        default:;
        }
        break;
    case SDL_WINDOWEVENT:
        if (event->window.event == SDL_WINDOWEVENT_RESIZED) {
            te->winW = event->window.data1;
            te->winH = event->window.data2;

            if (te->winW < MIN_WIN_WIDTH || te->winH < MIN_WIN_HEIGHT) {
                if (te->winW < MIN_WIN_WIDTH) te->winW = MIN_WIN_WIDTH;
                if (te->winH < MIN_WIN_HEIGHT) te->winH = MIN_WIN_HEIGHT;
                SDL_SetWindowSize(te->win, te->winW, te->winH);
            }

            te->groups = te->winW / te->groupWidth;
            if (te->groups < 1) te->groups = 1;
            te->charsPerRow = te->groups * 6;
        }
        break;
    default:;
    }
}

void _ternedit_update(TerneditState* te, float dt) {
    (void) te;
    (void) dt;
}

void _ternedit_draw(TerneditState* te) {
    SDL_SetRenderDrawColor(te->renderer, 0, 0, 0, 255);
    SDL_RenderClear(te->renderer);
    
    SDL_Rect rect = { .x = 0, .y = 0, .w = te->atlas.charWidth, .h = te->atlas.charHeight };
    for (size_t i = 0; i < te->bufSize; ++i) {
        SDL_Texture* tex = te->atlas.chars[REGULAR][te->buf[i]];
        if (i == te->cursorPos) {
            SDL_SetRenderDrawColor(te->renderer, 255, 255, 255, 255);
            SDL_RenderFillRect(te->renderer, &rect);
            SDL_SetTextureColorMod(tex, 0, 0, 0);
        } else {
            SDL_SetTextureColorMod(tex, 255, 255, 255);
        }
        SDL_RenderCopy(te->renderer, tex, NULL, &rect);
        rect.x += te->atlas.charWidth + ((i + 1) % 6 == 0 ? 17 : 0);
        if ((i + 1) % te->charsPerRow == 0) {
            rect.x = 0;
            rect.y += te->atlas.charHeight + 5;
        }
    }
    SDL_RenderPresent(te->renderer);
}

int ternedit_run(TerneditState* te) {
    uint32_t lastTick = SDL_GetTicks();
    while (te->running) {
        uint32_t currentTick = SDL_GetTicks();
        float dt = (currentTick - lastTick) / 1000.0f;
        lastTick = currentTick;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            _ternedit_handle(te, &event);
        }
        _ternedit_update(te, dt);
        _ternedit_draw(te);
    }
    return 0;
}
