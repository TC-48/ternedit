#include <ternedit.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>
#include <time.h>

void TerneditInit(TerneditState* te) {
    size_t initialSize = 10;
    TritState* initialBuf = (TritState*)malloc(initialSize);
    for (size_t i = 0; i < initialSize; ++i) {
        initialBuf[i] = rand() % 3;
    }
    te->doc = TernDocNew(initialBuf, initialSize);
    free(initialBuf);

    te->cursorPos = 0;
    te->running = SDL_TRUE;

    // TODO: this probably should be moved out of this function
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    te->win = SDL_CreateWindow("ternedit", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, DEFAULT_WIN_WIDTH, DEFAULT_WIN_HEIGHT, SDL_WINDOW_RESIZABLE);
    te->renderer = SDL_CreateRenderer(te->win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    te->editorFont.regular = TTF_OpenFont("assets/fonts/monogram/ttf/monogram.ttf", DEFAULT_FONT_SIZE);
    te->editorFont.italic = TTF_OpenFont("assets/fonts/monogram/ttf/monogram-extended-italic.ttf", DEFAULT_FONT_SIZE);

    te->atlas = LoadCharTextures(te->editorFont.regular, te->editorFont.italic, te->renderer);

    SDL_GetWindowSize(te->win, &te->winW, &te->winH);
    te->groupWidth = 6 * te->atlas.charWidth + 17;
    te->groups = te->winW / te->groupWidth;
    if (te->groups < 1) te->groups = 1;
    te->charsPerRow = (size_t) (te->groups * 6);

    te->scrollY = 0;
    te->scrollTargetY = 0;
}

void TerneditFree(TerneditState* te) {
    TernDocFree(te->doc);
    FreeCharTextures(&te->atlas);
    TTF_CloseFont(te->editorFont.regular);
    TTF_CloseFont(te->editorFont.italic);
    SDL_DestroyRenderer(te->renderer);
    SDL_DestroyWindow(te->win);

    // TODO: this probably should be moved out of this function
    TTF_Quit();
    SDL_Quit();
}

static void UpdateScroll(TerneditState* te) {
    int lineHeight = te->atlas.charHeight + 5;
    int cursorRow = te->cursorPos / te->charsPerRow;
    int cursorY = cursorRow * lineHeight;

    if (cursorY < te->scrollTargetY) {
        te->scrollTargetY = (float)cursorY;
    } else if (cursorY + lineHeight > te->scrollTargetY + te->winH) {
        te->scrollTargetY = (float)(cursorY + lineHeight - te->winH);
    }

    size_t docSize = TernDocSize(te->doc);
    int totalRows = (docSize + te->charsPerRow - 1) / te->charsPerRow;
    int totalHeight = totalRows * lineHeight;
    int maxScroll = totalHeight - te->winH;
    if (maxScroll < 0) maxScroll = 0;
    if (te->scrollTargetY > maxScroll) te->scrollTargetY = (float)maxScroll;
    if (te->scrollTargetY < 0) te->scrollTargetY = 0;
}

static void TerneditSave(TerneditState* te) {
    TernDocSave(te->doc, "out.t48b");
}

void _TerneditHandle(TerneditState* te, const SDL_Event* event) {
    size_t docSize = TernDocSize(te->doc);

    switch (event->type) {
    case SDL_QUIT:
        te->running = SDL_FALSE;
        break;
    case SDL_KEYDOWN: {
        SDL_Keymod mod = SDL_GetModState();
        SDL_bool ctrl = (mod & KMOD_CTRL) != 0;
        SDL_bool shift = (mod & KMOD_SHIFT) != 0;
        SDL_bool modified = SDL_FALSE;

        switch (event->key.keysym.scancode) {
        case SDL_SCANCODE_Z:
            if (ctrl) {
                if (shift) TernDocRedo(te->doc);
                else TernDocUndo(te->doc);
                docSize = TernDocSize(te->doc);
                if (te->cursorPos > docSize) te->cursorPos = docSize;
                modified = SDL_TRUE;
            }
            break;
        case SDL_SCANCODE_Y:
            if (ctrl) {
                TernDocRedo(te->doc);
                docSize = TernDocSize(te->doc);
                if (te->cursorPos > docSize) te->cursorPos = docSize;
                modified = SDL_TRUE;
            }
            break;
        case SDL_SCANCODE_LEFT:
            if (te->cursorPos > 0) te->cursorPos--;
            break;
        case SDL_SCANCODE_RIGHT:
            if (te->cursorPos < docSize) te->cursorPos++;
            break;
        case SDL_SCANCODE_UP:
            if (te->cursorPos >= te->charsPerRow) {
                te->cursorPos -= te->charsPerRow;
            }
            break;
        case SDL_SCANCODE_DOWN:
            if (te->cursorPos + te->charsPerRow <= docSize) {
                te->cursorPos += te->charsPerRow;
            } else {
                te->cursorPos = docSize;
            }
            break;
        case SDL_SCANCODE_BACKSPACE:
            if (te->cursorPos > 0) {
                TernDocDelete(te->doc, te->cursorPos - 1, 1);
                te->cursorPos--;
                modified = SDL_TRUE;
            }
            break;
        case SDL_SCANCODE_DELETE:
            if (te->cursorPos < docSize) {
                TernDocDelete(te->doc, te->cursorPos, 1);
                modified = SDL_TRUE;
            }
            break;
        case SDL_SCANCODE_0:
        case SDL_SCANCODE_1:
        case SDL_SCANCODE_2: {
            TritState val = 0;
            if (event->key.keysym.scancode == SDL_SCANCODE_1) val = 1;
            if (event->key.keysym.scancode == SDL_SCANCODE_2) val = 2;
            
            if (te->cursorPos < docSize) {
                TernDocReplace(te->doc, te->cursorPos, 1, sv_from_data_and_len((const char*)&val, 1));
            } else {
                TernDocInsert(te->doc, te->cursorPos, sv_from_data_and_len((const char*)&val, 1));
            }
            te->cursorPos++;
            modified = SDL_TRUE;
            break;
        }
        case SDL_SCANCODE_F: {
            if (te->cursorPos < docSize) {
                TritState cur = TernDocAt(te->doc, te->cursorPos);
                TritState next = (cur + 1) % 3;
                TernDocReplace(te->doc, te->cursorPos, 1, sv_from_data_and_len((const char*)&next, 1));
                modified = SDL_TRUE;
            }
            break;
        }
        case SDL_SCANCODE_INSERT: {
            // Random trit insertion for testing
            TritState r = rand() % 3;
            TernDocInsert(te->doc, te->cursorPos, sv_from_data_and_len((const char*)&r, 1));
            te->cursorPos++;
            modified = SDL_TRUE;
            break;
        }
        default:;
        }
        UpdateScroll(te);
        if (modified) TerneditSave(te);
    } break;

    case SDL_MOUSEWHEEL: {
        int lineHeight = te->atlas.charHeight + 5;
        te->scrollTargetY -= event->wheel.y * lineHeight * 3;
        
        int totalRows = (docSize + te->charsPerRow - 1) / te->charsPerRow;
        int totalHeight = totalRows * lineHeight;
        int maxScroll = totalHeight - te->winH;
        if (maxScroll < 0) maxScroll = 0;
        if (te->scrollTargetY > maxScroll) te->scrollTargetY = (float)maxScroll;
        if (te->scrollTargetY < 0) te->scrollTargetY = 0;
    } break;

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
            UpdateScroll(te);
        }
        break;
    default:;
    }
}

void _TerneditUpdate(TerneditState* te, float dt) {
    float scrollSpeed = 10.0f;
    te->scrollY += (te->scrollTargetY - te->scrollY) * dt * scrollSpeed;
}

void _TerneditDraw(TerneditState* te) {
    SDL_SetRenderDrawColor(te->renderer, 0, 0, 0, 255);
    SDL_RenderClear(te->renderer);
    
    size_t docSize = TernDocSize(te->doc);
    SDL_Rect rect = { .x = 0, .y = -(int)te->scrollY, .w = te->atlas.charWidth, .h = te->atlas.charHeight };

    for (size_t i = 0; i < docSize; ++i) {
        TritState val = TernDocAt(te->doc, i);
        SDL_Texture* tex = te->atlas.chars[REGULAR][val];
        
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

    if (te->cursorPos == docSize) {
        rect.w /= 5;
        SDL_SetRenderDrawColor(te->renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(te->renderer, &rect);
    }

    SDL_RenderPresent(te->renderer);
}

int TerneditRun(TerneditState* te) {
    uint32_t lastTick = SDL_GetTicks();
    while (te->running) {
        uint32_t currentTick = SDL_GetTicks();
        float dt = (currentTick - lastTick) / 1000.0f;
        lastTick = currentTick;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            _TerneditHandle(te, &event);
        }
        _TerneditUpdate(te, dt);
        _TerneditDraw(te);
    }
    return 0;
}
