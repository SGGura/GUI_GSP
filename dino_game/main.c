#include "lvgl/lvgl.h"
#include "lv_drivers/sdl/sdl.h"
#include "dino_game.h"

#include <SDL2/SDL.h>
#include <stdlib.h>
#include <unistd.h>

#define HOR_RES 800
#define VER_RES 400

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[HOR_RES * VER_RES / 10];
static lv_color_t buf2[HOR_RES * VER_RES / 10];

static lv_disp_drv_t disp_drv;
static lv_indev_drv_t kb_drv;

int main(void)
{
    lv_init();
    sdl_init();

    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, HOR_RES * VER_RES / 10);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res  = HOR_RES;
    disp_drv.ver_res  = VER_RES;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.flush_cb = sdl_display_flush;
    lv_disp_drv_register(&disp_drv);

    lv_indev_drv_init(&kb_drv);
    kb_drv.type    = LV_INDEV_TYPE_KEYPAD;
    kb_drv.read_cb = sdl_keyboard_read;
    lv_indev_drv_register(&kb_drv);

    lv_indev_drv_t mouse_drv;
    lv_indev_drv_init(&mouse_drv);
    mouse_drv.type    = LV_INDEV_TYPE_POINTER;
    mouse_drv.read_cb = sdl_mouse_read;
    lv_indev_drv_register(&mouse_drv);

    dino_game_create();

    while (1) {
        lv_timer_handler();
        SDL_Delay(5);
    }

    return 0;
}
