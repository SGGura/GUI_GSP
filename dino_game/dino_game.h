#ifndef DINO_GAME_H
#define DINO_GAME_H

#include "lvgl/lvgl.h"

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 400

#define GROUND_Y      320
#define GROUND_HEIGHT 2

#define DINO_WIDTH    44
#define DINO_HEIGHT   48
#define DINO_X        60
#define DINO_GROUND_Y (GROUND_Y - DINO_HEIGHT)

#define JUMP_VELOCITY    (-14)
#define GRAVITY          1

#define MAX_OBSTACLES    4
#define MAX_CLOUDS       6
#define MAX_STARS        20

#define INITIAL_SPEED    6
#define MAX_SPEED        14
#define SPEED_INCREMENT  0.001f

#define GAME_TICK_MS     16

typedef enum {
    OBSTACLE_NONE = 0,
    OBSTACLE_CACTUS_SMALL,
    OBSTACLE_CACTUS_LARGE,
    OBSTACLE_CACTUS_GROUP,
    OBSTACLE_BIRD
} obstacle_type_t;

typedef struct {
    obstacle_type_t type;
    float x;
    int y;
    int w;
    int h;
    lv_obj_t *obj;
    lv_color_t *buf;
    int bird_frame;
} obstacle_t;

typedef struct {
    float x;
    int y;
    lv_obj_t *obj;
    lv_color_t *buf;
} cloud_t;

typedef struct {
    int x;
    int y;
    lv_obj_t *obj;
} star_t;

typedef struct {
    bool running;
    bool game_over;
    bool ducking;
    bool night_mode;

    float dino_y;
    float dino_vy;
    bool  dino_on_ground;
    int   dino_frame;

    float speed;
    float distance;
    int   score;
    int   high_score;

    obstacle_t obstacles[MAX_OBSTACLES];
    cloud_t    clouds[MAX_CLOUDS];
    star_t     stars[MAX_STARS];

    float next_obstacle_dist;

    lv_obj_t   *screen;
    lv_obj_t   *ground_line;
    lv_obj_t   *dino_obj;
    lv_color_t *dino_buf;
    lv_obj_t   *score_label;
    lv_obj_t   *hi_score_label;
    lv_obj_t   *game_over_label;
    lv_obj_t   *restart_label;
    lv_obj_t   *start_label;

    lv_timer_t *game_timer;
    int frame_count;

    float ground_offset;
    lv_obj_t *ground_dots[40];
    int ground_dot_count;
} dino_game_t;

void dino_game_create(void);

#endif /* DINO_GAME_H */
