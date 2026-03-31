/**
 * @file lv_drv_conf.h
 * Configuration file for lv_drivers v8.3
 */

#ifndef LV_DRV_CONF_H
#define LV_DRV_CONF_H

#include "lv_conf.h"

/*********************
 * DELAY INTERFACE
 *********************/
#define LV_DRV_DELAY_INCLUDE <stdint.h>
#define LV_DRV_DELAY_US(us)
#define LV_DRV_DELAY_MS(ms)

/*********************
 * DISPLAY INTERFACE
 *********************/
#define LV_DRV_DISP_INCLUDE <stdint.h>
#define LV_DRV_DISP_CMD_DATA(val)
#define LV_DRV_DISP_SPI_CS(val)

/*********************
 *  INPUT INTERFACE
 *********************/
#define LV_DRV_INDEV_INCLUDE <stdint.h>
#define LV_DRV_INDEV_IRQ_READ 0

/*********************
 *  SDL DISPLAY
 *********************/
#ifndef USE_SDL
#define USE_SDL 1
#endif

#if USE_SDL
#define SDL_HOR_RES     800
#define SDL_VER_RES     400
#define SDL_ZOOM        1
#define SDL_INCLUDE_PATH <SDL2/SDL.h>
#define SDL_FULLSCREEN   0
#endif

/*********************
 *  SDL GPU
 *********************/
#ifndef USE_SDL_GPU
#define USE_SDL_GPU 0
#endif

#endif /* LV_DRV_CONF_H */
