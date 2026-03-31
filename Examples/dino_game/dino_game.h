/**
 * @file dino_game.h
 * @brief Chrome-style T-Rex dinosaur runner game for 128x64 monochrome displays.
 *
 * Platform-independent game logic. Call dino_init(), then dino_tick() every
 * frame (~15-30 ms) and draw with dino_draw().  Feed input via dino_jump().
 */
#ifndef DINO_GAME_H
#define DINO_GAME_H

#include <stdbool.h>
#include <stdint.h>

/* Display geometry (match GUI_GSP_Config defaults) */
#ifndef DINO_SCR_W
#define DINO_SCR_W  128
#endif
#ifndef DINO_SCR_H
#define DINO_SCR_H  64
#endif

/* Ground level (Y of ground line, from top) */
#define DINO_GROUND_Y   (DINO_SCR_H - 10)

/* Max simultaneous obstacles */
#define DINO_MAX_OBS    4

/* Max clouds */
#define DINO_MAX_CLOUDS 3

typedef enum {
    DINO_STATE_IDLE,
    DINO_STATE_RUNNING,
    DINO_STATE_JUMPING,
    DINO_STATE_DUCKING,
    DINO_STATE_DEAD
} DinoState;

typedef enum {
    OBS_NONE = 0,
    OBS_CACTUS_SMALL,
    OBS_CACTUS_TALL,
    OBS_CACTUS_GROUP,
    OBS_BIRD
} ObstacleKind;

typedef struct {
    ObstacleKind kind;
    int16_t x;
    int16_t y;
    int8_t  w;
    int8_t  h;
    int8_t  bird_frame;
} Obstacle;

typedef struct {
    int16_t x;
    int16_t y;
} Cloud;

typedef struct {
    /* Game state */
    DinoState state;
    bool      game_over;
    bool      started;

    /* Dino position/physics */
    int16_t dino_x;
    float   dino_y;
    float   dino_vy;
    bool    dino_ducking;
    uint8_t dino_frame;        /* animation frame for legs */

    /* Scrolling */
    float   speed;             /* pixels per tick */
    float   ground_offset;     /* for ground animation */
    uint32_t frame_count;

    /* Score */
    uint32_t score;
    uint32_t hi_score;

    /* Obstacles */
    Obstacle obs[DINO_MAX_OBS];

    /* Clouds */
    Cloud clouds[DINO_MAX_CLOUDS];

    /* Spawn control */
    int16_t next_spawn_dist;

    /* Night mode */
    bool night;
} DinoGame;

/* ---- Public API ---- */

void dino_init(DinoGame *g);

/**
 * Advance game by one tick (~16-33 ms depending on refresh rate).
 * @param dt_ms  elapsed time since last tick in milliseconds.
 */
void dino_tick(DinoGame *g, int dt_ms);

/** Signal a jump / restart (button press). */
void dino_jump(DinoGame *g);

/** Signal duck start (hold down). */
void dino_duck_start(DinoGame *g);

/** Signal duck end (release down). */
void dino_duck_end(DinoGame *g);

/* ---- Drawing callback interface ---- */

/**
 * The game uses a callback to set individual pixels so it can work with
 * any display back-end (GUI_GSP PutPixel, SDL, framebuffer, etc.).
 */
typedef void (*DinoSetPixelFn)(int x, int y, int color);
typedef void (*DinoFillRectFn)(int x, int y, int w, int h, int color);

typedef struct {
    DinoSetPixelFn  set_pixel;   /* required */
    DinoFillRectFn  fill_rect;   /* optional; fallback via set_pixel */
} DinoDraw;

void dino_draw(const DinoGame *g, const DinoDraw *draw);

#endif /* DINO_GAME_H */
