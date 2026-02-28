#include <SDL2/SDL_render.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_ttf.h>

#include <ch-atlas.h>

#include <unistd.h>

#define DEFAULT_WIN_WIDTH 680
#define DEFAULT_WIN_HEIGHT 480

#define MIN_WIN_WIDTH 200
#define MIN_WIN_HEIGHT 150

#define DEFAULT_FONT_SIZE 88

typedef uint8_t TritState;

int main() {
    TritState data[1024];
    size_t count = 80;
    for (size_t i = 0; i < count; ++i) {
        data[i] = rand() % 3;
    }

    size_t cursorPos = 0;

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    SDL_Window* window = SDL_CreateWindow("ternedit", 0, 0, DEFAULT_WIN_WIDTH, DEFAULT_WIN_HEIGHT, SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);

    TTF_Font* regular = TTF_OpenFont("assets/fonts/monogram/ttf/monogram.ttf", DEFAULT_FONT_SIZE);
    if (!regular) return 1;

    TTF_Font* italic = TTF_OpenFont("assets/fonts/monogram/ttf/monogram-extended-italic.ttf", DEFAULT_FONT_SIZE);
    if (!italic) return 1;

    CharTextureAtlas atlas = LoadCharTextures(regular, italic, renderer);

    int winW, winH;
    SDL_GetWindowSize(window, &winW, &winH);
    int groupWidth = 6 * atlas.charWidth + 17;
    int groups = winW / groupWidth;
    if (groups < 1) groups = 1;
    size_t charsPerRow = (size_t) (groups * 6);

    SDL_bool running = SDL_TRUE;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                running = SDL_FALSE;
                break;
            case SDL_KEYUP:
                switch (event.key.keysym.scancode) {
                case SDL_SCANCODE_LEFT:
                    if (cursorPos > 0) cursorPos--;
                    break;
                case SDL_SCANCODE_RIGHT:
                    if (cursorPos + 1 < count) cursorPos++;
                    break;
                case SDL_SCANCODE_UP:
                    if (cursorPos >= charsPerRow) {
                        cursorPos -= charsPerRow;
                    }
                    break;
                case SDL_SCANCODE_DOWN:
                    if (cursorPos + charsPerRow < count) {
                        cursorPos += charsPerRow;
                    }
                    break;
                default:;
                }
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    int w = event.window.data1;
                    int h = event.window.data2;

                    // enforce minimum size
                    if (w < MIN_WIN_WIDTH || h < MIN_WIN_HEIGHT) {
                        if (w < MIN_WIN_WIDTH) w = MIN_WIN_WIDTH;
                        if (h < MIN_WIN_HEIGHT) h = MIN_WIN_HEIGHT;
                        SDL_SetWindowSize(window, w, h);
                    }

                    groups = w / groupWidth;
                    if (groups < 1) groups = 1;
                    charsPerRow = groups * 6;
                }
                break;
            default:;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_Rect rect = { .x = 0, .y = 0, .w = atlas.charWidth, .h = atlas.charHeight };
        for (size_t i = 0; i < count; ++i) {
            SDL_Texture* tex = atlas.chars[REGULAR][data[i]];
            if (i == cursorPos) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderFillRect(renderer, &rect);
                SDL_SetTextureColorMod(tex, 0, 0, 0);
            } else {
                SDL_SetTextureColorMod(tex, 255, 255, 255);
            }
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            rect.x += atlas.charWidth + ((i+1) % 6 == 0 ? 17 : 0);
            if ((i+1) % charsPerRow == 0) {
                rect.x = 0;
                rect.y += atlas.charHeight + 5;
            }
        }
        SDL_RenderPresent(renderer);
    }
    
    FreeCharTextures(&atlas);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    TTF_Quit();
    return 0;
}
