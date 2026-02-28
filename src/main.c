#include <SDL2/SDL.h>

#include <ch-atlas.h>

#include <unistd.h>

#define DEFAULT_WIN_WIDTH 640
#define DEFAULT_WIN_HEIGHT 480

#define DEFAULT_FONT_SIZE 88

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    SDL_Window* window = SDL_CreateWindow("ternedit", 0, 0, DEFAULT_WIN_WIDTH, DEFAULT_WIN_HEIGHT, 0);
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
            default:;
            }
        }


        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, atlas.chars[REGULAR][0], NULL, &(SDL_Rect) { .x = 0, .y = 0, .w = atlas.charWidth, .h = atlas.charHeight });
        SDL_RenderPresent(renderer);
    }
    
    FreeCharTextures(&atlas);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    TTF_Quit();
    return 0;
}
