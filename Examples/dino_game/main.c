/**
 * @file main.c
 * @brief Entry point for the Chrome-style T-Rex dinosaur runner game.
 *
 * Integration:
 *  1. Add GUI_GSP sources and your display driver to the project (see PORTING_GUIDE).
 *  2. Add dino_game.c and this main.c to the build.
 *  3. In your timer ISR (e.g. every 10 ms) call GUI_TimerClock(10).
 *  4. Map DINO_KEY_JUMP, DINO_KEY_DUCK, DINO_KEY_OK to your hardware key codes
 *     (defaults: 1, 2, 3) — either via #define before including dino_game.h
 *     or in your Keyboard.h / key_poll / read_key implementation.
 *
 * The game creates its own GSP Screen and a 50 ms periodic timer.
 * All rendering and logic happen inside the timer callback;
 * the main loop only calls GUI_Run() which drives the timer subsystem.
 */

#include "GUI_GSP/GUI_GSP.h"
#include "dino_game.h"

int main(void)
{
    GUI_GSP_Init();

    DinoGame_Create(NULL);

    for (;;) {
        GUI_Run();
    }
}
