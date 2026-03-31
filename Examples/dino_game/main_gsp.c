/**
 * @file main_gsp.c
 * @brief T-Rex dino game integration with GUI_GSP library for embedded targets.
 *
 * Integration steps:
 * 1. Add this file + dino_game.c/h to your project.
 * 2. Set up GSP_HW (display, keyboard) in GSP_Hardware.c as usual.
 * 3. In your timer ISR call GUI_TimerClock(period_ms).
 * 4. Key mapping: define DINO_KEY_JUMP and DINO_KEY_DUCK to match your keypad.
 *
 * The game renders directly via GSP_Driver primitives (PutPixel, Bar).
 */

#include "GUI_GSP/GUI_GSP.h"
#include "dino_game.h"

#ifndef DINO_KEY_JUMP
#define DINO_KEY_JUMP  1   /* map to your keypad codes */
#endif
#ifndef DINO_KEY_DUCK
#define DINO_KEY_DUCK  2
#endif

#define GAME_TICK_MS  33   /* ~30 FPS */

static DinoGame game;
static _GSP_Screen *GameScreen;

static void gsp_set_pixel(int x, int y, int color)
{
    if (x < 0 || x >= SCREEN_HOR_SIZE || y < 0 || y >= SCREEN_VER_SIZE) return;
    SetColor(color ? BLACK : WHITE);
    PutPixel((short)x, (short)y);
}

static void gsp_fill_rect(int x, int y, int w, int h, int color)
{
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_HOR_SIZE) w = SCREEN_HOR_SIZE - x;
    if (y + h > SCREEN_VER_SIZE) h = SCREEN_VER_SIZE - y;
    if (w <= 0 || h <= 0) return;
    SetColor(color ? BLACK : WHITE);
    Bar((short)x, (short)y, (short)(x + w - 1), (short)(y + h - 1));
}

static const DinoDraw gsp_draw = {
    .set_pixel = gsp_set_pixel,
    .fill_rect = gsp_fill_rect,
};

static void GameKeyHandler(void)
{
    int key = GSP_ReadKey();
    if (key == DINO_KEY_JUMP)
        dino_jump(&game);
    else if (key == DINO_KEY_DUCK)
        dino_duck_start(&game);
    else
        dino_duck_end(&game);
}

static void GameTimerCallback(void *timer)
{
    (void)timer;
    dino_tick(&game, GAME_TICK_MS);
}

static void CreateGameScreen(void)
{
    GameScreen = Crate_Screen(0, 0, SCREEN_HOR_SIZE, SCREEN_VER_SIZE, 0);
    if (!GameScreen) return;

    ScreenSetKeyHandler(GameScreen, GameKeyHandler);

#ifdef USED_TIMER
    CreateTimer(GameScreen, GameTimerCallback, GAME_TICK_MS, -1);
#endif

    Set_Active_Screen(GameScreen);
    dino_init(&game);
}

int main(void)
{
    GUI_GSP_Init();
    CreateGameScreen();

    for (;;) {
        GUI_Run();

        /* If not using GSP timers, tick the game manually every refresh */
#ifndef USED_TIMER
        dino_tick(&game, GUI_REFR_PERIOD);
#endif

        dino_draw(&game, &gsp_draw);
        Refresh_GSP();
    }
}
