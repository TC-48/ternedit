#include <SDL2/SDL.h>

#include <ch-atlas.h>

#include <unistd.h>

#define DEFAULT_WIN_WIDTH 640
#define DEFAULT_WIN_HEIGHT 480

#define MIN_WIN_WIDTH 200
#define MIN_WIN_HEIGHT 150

#define DEFAULT_FONT_SIZE 88

typedef uint8_t TritState;

int main() {
    TritState data[1024] = { 1, 0, 2, 1, 0, 1, 1, 2, 0, 1, 2, 2, 2, 1, 0, 2 };
    size_t count = 16;

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    SDL_Window* window = SDL_CreateWindow("ternedit", 0, 0, DEFAULT_WIN_WIDTH, DEFAULT_WIN_HEIGHT, SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    TTF_Font* regular = TTF_OpenFont("assets/fonts/monogram/ttf/monogram.ttf", DEFAULT_FONT_SIZE);
    if (!regular) return 1;

    TTF_Font* italic = TTF_OpenFont("assets/fonts/monogram/ttf/monogram-extended-italic.ttf", DEFAULT_FONT_SIZE);
    if (!italic) return 1;

    CharTextureAtlas atlas = LoadCharTextures(regular, italic, renderer);

    SDL_bool running = SDL_TRUE;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                running = SDL_FALSE;
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
                }
                break;
            default:;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_Rect rect = { .x = 0, .y = 0, .w = atlas.charWidth, .h = atlas.charHeight };
        for (size_t i = 0; i < count; ++i) {
            SDL_RenderCopy(renderer, atlas.chars[REGULAR][data[i]], NULL, &rect);
            rect.x += atlas.charWidth + ((i+1) % 6 == 0 ? 17 : 0);
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
