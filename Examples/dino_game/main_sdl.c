/**
 * @file main_sdl.c
 * @brief SDL2 PC simulator for the Chrome T-Rex dino game.
 *
 * Build:
 *   gcc -O2 -o dino_sdl main_sdl.c dino_game.c -lSDL2
 *
 * Controls:
 *   SPACE / UP   — jump / restart
 *   DOWN         — duck
 *   ESC / Q      — quit
 */
#include <SDL2/SDL.h>
#include "dino_game.h"

#define SCALE   6        /* each game pixel → 6×6 screen pixels */
#define WIN_W   (DINO_SCR_W * SCALE)
#define WIN_H   (DINO_SCR_H * SCALE)
#define FPS     30

static SDL_Renderer *renderer;

static void sdl_set_pixel(int x, int y, int color)
{
    if (x < 0 || x >= DINO_SCR_W || y < 0 || y >= DINO_SCR_H) return;
    if (color)
        SDL_SetRenderDrawColor(renderer, 0x53, 0x53, 0x53, 0xFF);
    else
        SDL_SetRenderDrawColor(renderer, 0xF7, 0xF7, 0xF7, 0xFF);

    SDL_Rect r = { x * SCALE, y * SCALE, SCALE, SCALE };
    SDL_RenderFillRect(renderer, &r);
}

static void sdl_fill_rect(int x, int y, int w, int h, int color)
{
    if (color)
        SDL_SetRenderDrawColor(renderer, 0x53, 0x53, 0x53, 0xFF);
    else
        SDL_SetRenderDrawColor(renderer, 0xF7, 0xF7, 0xF7, 0xFF);

    SDL_Rect r = { x * SCALE, y * SCALE, w * SCALE, h * SCALE };
    SDL_RenderFillRect(renderer, &r);
}

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow(
        "Dino T-Rex (GUI_GSP)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIN_W, WIN_H, SDL_WINDOW_SHOWN);
    if (!win) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    DinoGame game;
    dino_init(&game);

    DinoDraw draw_cb = {
        .set_pixel = sdl_set_pixel,
        .fill_rect = sdl_fill_rect,
    };

    bool running = true;
    Uint32 last_tick = SDL_GetTicks();

    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_KEYDOWN:
                if (ev.key.repeat) break;
                switch (ev.key.keysym.sym) {
                case SDLK_ESCAPE:
                case SDLK_q:
                    running = false;
                    break;
                case SDLK_SPACE:
                case SDLK_UP:
                    dino_jump(&game);
                    break;
                case SDLK_DOWN:
                    dino_duck_start(&game);
                    break;
                default:
                    break;
                }
                break;
            case SDL_KEYUP:
                if (ev.key.keysym.sym == SDLK_DOWN)
                    dino_duck_end(&game);
                break;
            }
        }

        Uint32 now = SDL_GetTicks();
        int dt = (int)(now - last_tick);
        if (dt >= (1000 / FPS)) {
            last_tick = now;
            dino_tick(&game, dt);
            dino_draw(&game, &draw_cb);
            SDL_RenderPresent(renderer);
        } else {
            SDL_Delay(1);
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
