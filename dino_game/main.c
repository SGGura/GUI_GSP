#include "lvgl.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

#define SCREEN_W  800
#define SCREEN_H  400

#define GROUND_Y         320

#define DINO_W           44
#define DINO_H           48
#define DINO_X           80
#define DINO_DUCK_H      30
#define DINO_DUCK_W      58

#define CACTUS_SMALL_W   17
#define CACTUS_SMALL_H   35
#define CACTUS_BIG_W     25
#define CACTUS_BIG_H     50

#define BIRD_W           46
#define BIRD_H           30

#define MAX_OBSTACLES    4

#define JUMP_VELOCITY    (-14.0f)
#define GRAVITY          (0.65f)
#define FAST_FALL_GRAVITY (1.3f)

#define INITIAL_SPEED    6.0f
#define MAX_SPEED        14.0f
#define SPEED_INCREMENT  0.003f

#define OBSTACLE_MIN_GAP 250
#define OBSTACLE_MAX_GAP 450

#define CLOUD_COUNT      5
#define STAR_COUNT        15

typedef enum {
    OBS_NONE,
    OBS_CACTUS_SMALL,
    OBS_CACTUS_BIG,
    OBS_CACTUS_CLUSTER,
    OBS_BIRD_LOW,
    OBS_BIRD_HIGH,
} obstacle_type_t;

typedef struct {
    obstacle_type_t type;
    float x;
    int w, h;
    int y;
    bool active;
} obstacle_t;

typedef struct {
    float x, y;
    int w, h;
} cloud_t;

typedef struct {
    int x, y;
    int w;
} ground_detail_t;

typedef enum {
    STATE_IDLE,
    STATE_RUNNING,
    STATE_GAMEOVER,
} game_state_t;

static lv_obj_t *scr;
static lv_obj_t *canvas;
static lv_obj_t *score_label;
static lv_obj_t *hi_score_label;
static lv_obj_t *gameover_label;
static lv_obj_t *restart_hint;
static lv_obj_t *start_hint;

static obstacle_t obstacles[MAX_OBSTACLES];
static cloud_t clouds[CLOUD_COUNT];
static ground_detail_t ground_details[30];

static float dino_y;
static float dino_vy;
static bool  dino_jumping;
static bool  dino_ducking;
static int   dino_duck_frames;
static int   dino_anim_frame;
static int   dino_anim_counter;

static float game_speed;
static int   score;
static int   hi_score;
static game_state_t game_state;

static float next_obstacle_dist;
static int   frame_counter;
static bool  night_mode;
static int   night_timer;

static lv_timer_t *game_timer;

static uint8_t *canvas_buf;
static lv_layer_t layer;

static void draw_game(void);
static void spawn_obstacle(void);
static bool check_collision(void);
static void game_loop_cb(lv_timer_t *timer);
static void start_game(void);
static void game_over(void);

static lv_color_t bg_color(void)      { return night_mode ? lv_color_hex(0x1a1a2e) : lv_color_hex(0xF7F7F7); }
static lv_color_t fg_color(void)      { return night_mode ? lv_color_hex(0xE0E0E0) : lv_color_hex(0x535353); }
static lv_color_t ground_color(void)  { return night_mode ? lv_color_hex(0x888888) : lv_color_hex(0x535353); }
static lv_color_t cloud_color(void)   { return night_mode ? lv_color_hex(0x333355) : lv_color_hex(0xDDDDDD); }
static lv_color_t detail_color(void)  { return night_mode ? lv_color_hex(0x555555) : lv_color_hex(0xAAAAAA); }

static void fill_rect(lv_draw_buf_t *draw_buf, int x, int y, int w, int h, lv_color_t color) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (y + h > SCREEN_H) h = SCREEN_H - y;
    if (w <= 0 || h <= 0) return;

    uint32_t stride = draw_buf->header.stride;
    uint8_t *data = draw_buf->data;

    for (int row = y; row < y + h; row++) {
        uint32_t *px = (uint32_t *)(data + row * stride) + x;
        uint32_t c = lv_color_to_u32(color) | 0xFF000000;
        for (int col = 0; col < w; col++) {
            px[col] = c;
        }
    }
}

/* --- Dino pixel art drawing --- */
static void draw_dino_standing(lv_draw_buf_t *db, int ox, int oy) {
    lv_color_t dk = fg_color();

    fill_rect(db, ox+18, oy+0,  22, 4,  dk);
    fill_rect(db, ox+14, oy+4,  30, 4,  dk);
    fill_rect(db, ox+22, oy+4,  4,  4,  lv_color_white());
    fill_rect(db, ox+10, oy+8,  30, 4,  dk);
    fill_rect(db, ox+10, oy+12, 20, 4,  dk);
    fill_rect(db, ox+6,  oy+16, 12, 4,  dk);
    fill_rect(db, ox+0,  oy+20, 24, 4,  dk);
    fill_rect(db, ox+6,  oy+24, 14, 4,  dk);
    fill_rect(db, ox+10, oy+28, 10, 4,  dk);
    fill_rect(db, ox+10, oy+32, 14, 4,  dk);
    fill_rect(db, ox+10, oy+36, 10, 4,  dk);
    fill_rect(db, ox+10, oy+40, 4,  8,  dk);
    fill_rect(db, ox+22, oy+40, 4,  8,  dk);
}

static void draw_dino_running(lv_draw_buf_t *db, int ox, int oy, int frame) {
    lv_color_t dk = fg_color();

    fill_rect(db, ox+18, oy+0,  22, 4,  dk);
    fill_rect(db, ox+14, oy+4,  30, 4,  dk);
    fill_rect(db, ox+22, oy+4,  4,  4,  lv_color_white());
    fill_rect(db, ox+10, oy+8,  30, 4,  dk);
    fill_rect(db, ox+10, oy+12, 20, 4,  dk);
    fill_rect(db, ox+6,  oy+16, 12, 4,  dk);
    fill_rect(db, ox+0,  oy+20, 24, 4,  dk);
    fill_rect(db, ox+6,  oy+24, 14, 4,  dk);
    fill_rect(db, ox+10, oy+28, 10, 4,  dk);
    fill_rect(db, ox+10, oy+32, 14, 4,  dk);
    fill_rect(db, ox+10, oy+36, 10, 4,  dk);

    if (frame == 0) {
        fill_rect(db, ox+10, oy+40, 4, 8,  dk);
        fill_rect(db, ox+22, oy+40, 4, 4,  dk);
    } else {
        fill_rect(db, ox+10, oy+40, 4, 4,  dk);
        fill_rect(db, ox+22, oy+40, 4, 8,  dk);
    }
}

static void draw_dino_ducking(lv_draw_buf_t *db, int ox, int oy, int frame) {
    lv_color_t dk = fg_color();

    fill_rect(db, ox+34, oy+0,  22, 4,  dk);
    fill_rect(db, ox+30, oy+4,  28, 4,  dk);
    fill_rect(db, ox+38, oy+4,  4,  4,  lv_color_white());
    fill_rect(db, ox+0,  oy+8,  54, 4,  dk);
    fill_rect(db, ox+6,  oy+12, 30, 4,  dk);
    fill_rect(db, ox+10, oy+16, 22, 4,  dk);
    fill_rect(db, ox+10, oy+20, 14, 4,  dk);

    if (frame == 0) {
        fill_rect(db, ox+10, oy+24, 4, 6, dk);
        fill_rect(db, ox+20, oy+24, 4, 2, dk);
    } else {
        fill_rect(db, ox+10, oy+24, 4, 2, dk);
        fill_rect(db, ox+20, oy+24, 4, 6, dk);
    }
}

static void draw_dino_dead(lv_draw_buf_t *db, int ox, int oy) {
    lv_color_t dk = fg_color();

    fill_rect(db, ox+18, oy+0,  22, 4,  dk);
    fill_rect(db, ox+14, oy+4,  30, 4,  dk);
    /* X eyes */
    fill_rect(db, ox+20, oy+4,  3,  1,  lv_color_white());
    fill_rect(db, ox+21, oy+5,  1,  1,  lv_color_white());
    fill_rect(db, ox+20, oy+6,  3,  1,  lv_color_white());
    fill_rect(db, ox+10, oy+8,  30, 4,  dk);
    fill_rect(db, ox+10, oy+12, 20, 4,  dk);
    fill_rect(db, ox+6,  oy+16, 12, 4,  dk);
    fill_rect(db, ox+0,  oy+20, 24, 4,  dk);
    fill_rect(db, ox+6,  oy+24, 14, 4,  dk);
    fill_rect(db, ox+10, oy+28, 10, 4,  dk);
    fill_rect(db, ox+10, oy+32, 14, 4,  dk);
    fill_rect(db, ox+10, oy+36, 10, 4,  dk);
    fill_rect(db, ox+10, oy+40, 4,  8,  dk);
    fill_rect(db, ox+22, oy+40, 4,  8,  dk);
}

static void draw_cactus_small(lv_draw_buf_t *db, int ox, int oy) {
    lv_color_t c = fg_color();
    fill_rect(db, ox+5,  oy+0,  7, 20, c);
    fill_rect(db, ox+0,  oy+7,  5, 10, c);
    fill_rect(db, ox+12, oy+5,  5, 10, c);
    fill_rect(db, ox+5,  oy+20, 7, 15, c);
}

static void draw_cactus_big(lv_draw_buf_t *db, int ox, int oy) {
    lv_color_t c = fg_color();
    fill_rect(db, ox+8,  oy+0,  9, 30, c);
    fill_rect(db, ox+0,  oy+10, 8, 12, c);
    fill_rect(db, ox+17, oy+6,  8, 12, c);
    fill_rect(db, ox+8,  oy+30, 9, 20, c);
}

static void draw_cactus_cluster(lv_draw_buf_t *db, int ox, int oy) {
    lv_color_t c = fg_color();
    for (int i = 0; i < 3; i++) {
        int cx = ox + i * CACTUS_SMALL_W;
        int cy = oy + ((i == 1) ? 0 : 5);
        fill_rect(db, cx+5,  cy+0,  7, 20, c);
        fill_rect(db, cx+0,  cy+7,  5, 10, c);
        fill_rect(db, cx+12, cy+5,  5, 10, c);
        fill_rect(db, cx+5,  cy+20, 7, 15, c);
    }
}

static void draw_bird(lv_draw_buf_t *db, int ox, int oy, int frame) {
    lv_color_t c = fg_color();
    fill_rect(db, ox+4,  oy+14, 38, 6, c);
    fill_rect(db, ox+0,  oy+12, 8,  8, c);
    if (frame == 0) {
        fill_rect(db, ox+10, oy+6, 6, 8, c);
        fill_rect(db, ox+16, oy+2, 6, 6, c);
    } else {
        fill_rect(db, ox+10, oy+20, 6, 8, c);
        fill_rect(db, ox+16, oy+24, 6, 6, c);
    }
}

static void draw_game(void) {
    lv_draw_buf_t *draw_buf = lv_canvas_get_draw_buf(canvas);

    lv_color_t bg = bg_color();
    uint32_t bg32 = lv_color_to_u32(bg) | 0xFF000000;
    uint32_t stride = draw_buf->header.stride;
    uint8_t *data = draw_buf->data;
    for (int row = 0; row < SCREEN_H; row++) {
        uint32_t *px = (uint32_t *)(data + row * stride);
        for (int col = 0; col < SCREEN_W; col++) {
            px[col] = bg32;
        }
    }

    for (int i = 0; i < CLOUD_COUNT; i++) {
        int cx = (int)clouds[i].x;
        int cy = (int)clouds[i].y;
        fill_rect(draw_buf, cx, cy+4, clouds[i].w, clouds[i].h-8, cloud_color());
        fill_rect(draw_buf, cx+10, cy, clouds[i].w-20, clouds[i].h, cloud_color());
    }

    if (night_mode) {
        lv_color_t star_c = lv_color_hex(0xCCCCCC);
        srand(42);
        for (int i = 0; i < 20; i++) {
            int sx = rand() % SCREEN_W;
            int sy = 20 + rand() % 150;
            fill_rect(draw_buf, sx, sy, 2, 2, star_c);
        }
        srand((unsigned)time(NULL) + frame_counter);
    }

    fill_rect(draw_buf, 0, GROUND_Y, SCREEN_W, 2, ground_color());

    for (int i = 0; i < 30; i++) {
        int gx = ground_details[i].x - (int)(game_speed * frame_counter * 0.5f) % SCREEN_W;
        while (gx < 0) gx += SCREEN_W;
        gx %= SCREEN_W;
        fill_rect(draw_buf, gx, ground_details[i].y, ground_details[i].w, 1, detail_color());
    }

    int dino_draw_h, dino_draw_w;
    int dino_draw_y;

    if (game_state == STATE_GAMEOVER) {
        dino_draw_h = DINO_H;
        dino_draw_w = DINO_W;
        dino_draw_y = (int)dino_y - dino_draw_h;
        draw_dino_dead(draw_buf, DINO_X, dino_draw_y);
    } else if (dino_ducking && !dino_jumping) {
        dino_draw_h = DINO_DUCK_H;
        dino_draw_w = DINO_DUCK_W;
        dino_draw_y = (int)dino_y - dino_draw_h;
        draw_dino_ducking(draw_buf, DINO_X, dino_draw_y, dino_anim_frame);
    } else if (game_state == STATE_RUNNING && !dino_jumping) {
        dino_draw_h = DINO_H;
        dino_draw_w = DINO_W;
        dino_draw_y = (int)dino_y - dino_draw_h;
        draw_dino_running(draw_buf, DINO_X, dino_draw_y, dino_anim_frame);
    } else {
        dino_draw_h = DINO_H;
        dino_draw_w = DINO_W;
        dino_draw_y = (int)dino_y - dino_draw_h;
        draw_dino_standing(draw_buf, DINO_X, dino_draw_y);
    }

    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!obstacles[i].active) continue;
        obstacle_t *obs = &obstacles[i];
        int ox = (int)obs->x;
        int oy = obs->y;

        switch (obs->type) {
            case OBS_CACTUS_SMALL:
                draw_cactus_small(draw_buf, ox, oy);
                break;
            case OBS_CACTUS_BIG:
                draw_cactus_big(draw_buf, ox, oy);
                break;
            case OBS_CACTUS_CLUSTER:
                draw_cactus_cluster(draw_buf, ox, oy);
                break;
            case OBS_BIRD_LOW:
            case OBS_BIRD_HIGH:
                draw_bird(draw_buf, ox, oy, dino_anim_frame);
                break;
            default:
                break;
        }
    }

    lv_obj_invalidate(canvas);
}

static void spawn_obstacle(void) {
    int slot = -1;
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!obstacles[i].active) { slot = i; break; }
    }
    if (slot < 0) return;

    obstacle_t *obs = &obstacles[slot];
    obs->active = true;
    obs->x = SCREEN_W + 20;

    int r = rand() % 100;
    if (score > 200 && r < 20) {
        if (rand() % 2 == 0) {
            obs->type = OBS_BIRD_LOW;
            obs->w = BIRD_W; obs->h = BIRD_H;
            obs->y = GROUND_Y - BIRD_H - 5;
        } else {
            obs->type = OBS_BIRD_HIGH;
            obs->w = BIRD_W; obs->h = BIRD_H;
            obs->y = GROUND_Y - 80;
        }
    } else if (r < 40) {
        obs->type = OBS_CACTUS_SMALL;
        obs->w = CACTUS_SMALL_W; obs->h = CACTUS_SMALL_H;
        obs->y = GROUND_Y - CACTUS_SMALL_H;
    } else if (r < 70) {
        obs->type = OBS_CACTUS_BIG;
        obs->w = CACTUS_BIG_W; obs->h = CACTUS_BIG_H;
        obs->y = GROUND_Y - CACTUS_BIG_H;
    } else {
        obs->type = OBS_CACTUS_CLUSTER;
        obs->w = CACTUS_SMALL_W * 3; obs->h = CACTUS_SMALL_H + 5;
        obs->y = GROUND_Y - obs->h;
    }
}

static bool check_collision(void) {
    int dino_cy;
    int dino_cw, dino_ch;

    if (dino_ducking && !dino_jumping) {
        dino_cw = DINO_DUCK_W - 10;
        dino_ch = DINO_DUCK_H - 4;
        dino_cy = (int)dino_y - DINO_DUCK_H;
    } else {
        dino_cw = DINO_W - 10;
        dino_ch = DINO_H - 8;
        dino_cy = (int)dino_y - DINO_H;
    }
    int dino_left   = DINO_X + 5;
    int dino_right  = DINO_X + dino_cw;
    int dino_top    = dino_cy + 4;
    int dino_bottom = dino_cy + dino_ch;

    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!obstacles[i].active) continue;
        obstacle_t *obs = &obstacles[i];

        int obs_left   = (int)obs->x + 3;
        int obs_right  = (int)obs->x + obs->w - 3;
        int obs_top    = obs->y + 3;
        int obs_bottom = obs->y + obs->h - 3;

        if (dino_left < obs_right && dino_right > obs_left &&
            dino_top < obs_bottom && dino_bottom > obs_top) {
            return true;
        }
    }
    return false;
}

static void start_game(void) {
    game_state = STATE_RUNNING;
    score = 0;
    game_speed = INITIAL_SPEED;
    dino_y = GROUND_Y;
    dino_vy = 0;
    dino_jumping = false;
    dino_ducking = false;
    dino_duck_frames = 0;
    dino_anim_frame = 0;
    dino_anim_counter = 0;
    frame_counter = 0;
    next_obstacle_dist = 300;
    night_mode = false;
    night_timer = 0;

    for (int i = 0; i < MAX_OBSTACLES; i++) {
        obstacles[i].active = false;
    }

    lv_obj_add_flag(gameover_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(restart_hint, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(start_hint, LV_OBJ_FLAG_HIDDEN);

    char buf[32];
    snprintf(buf, sizeof(buf), "%05d", score);
    lv_label_set_text(score_label, buf);
}

static void game_over(void) {
    game_state = STATE_GAMEOVER;
    if (score > hi_score) hi_score = score;

    char buf[32];
    snprintf(buf, sizeof(buf), "HI %05d", hi_score);
    lv_label_set_text(hi_score_label, buf);

    lv_obj_remove_flag(gameover_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(restart_hint, LV_OBJ_FLAG_HIDDEN);
}

static void game_loop_cb(lv_timer_t *timer) {
    (void)timer;

    if (game_state == STATE_IDLE) {
        draw_game();
        return;
    }

    if (game_state == STATE_GAMEOVER) {
        draw_game();
        return;
    }

    frame_counter++;

    if (dino_jumping) {
        float grav = dino_ducking ? FAST_FALL_GRAVITY : GRAVITY;
        dino_vy += grav;
        dino_y += dino_vy;
        if (dino_y >= GROUND_Y) {
            dino_y = GROUND_Y;
            dino_vy = 0;
            dino_jumping = false;
        }
    }

    if (dino_duck_frames > 0) {
        dino_duck_frames--;
        if (dino_duck_frames == 0) dino_ducking = false;
    }

    dino_anim_counter++;
    if (dino_anim_counter >= 6) {
        dino_anim_counter = 0;
        dino_anim_frame = 1 - dino_anim_frame;
    }

    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!obstacles[i].active) continue;
        obstacles[i].x -= game_speed;
        if (obstacles[i].x < -100) {
            obstacles[i].active = false;
        }
    }

    next_obstacle_dist -= game_speed;
    if (next_obstacle_dist <= 0) {
        spawn_obstacle();
        next_obstacle_dist = OBSTACLE_MIN_GAP + (rand() % (OBSTACLE_MAX_GAP - OBSTACLE_MIN_GAP));
        float speed_factor = game_speed / INITIAL_SPEED;
        next_obstacle_dist *= speed_factor;
    }

    for (int i = 0; i < CLOUD_COUNT; i++) {
        clouds[i].x -= game_speed * 0.3f;
        if (clouds[i].x + clouds[i].w < 0) {
            clouds[i].x = SCREEN_W + rand() % 100;
            clouds[i].y = 40 + rand() % 100;
        }
    }

    if (check_collision()) {
        game_over();
        draw_game();
        return;
    }

    score++;

    night_timer++;
    if (night_timer >= 700) {
        night_mode = !night_mode;
        night_timer = 0;

        lv_color_t txt = night_mode ? lv_color_hex(0xE0E0E0) : lv_color_hex(0x535353);
        lv_color_t bg = night_mode ? lv_color_hex(0x1a1a2e) : lv_color_hex(0xF7F7F7);
        lv_obj_set_style_bg_color(scr, bg, 0);
        lv_obj_set_style_text_color(score_label, txt, 0);
        lv_obj_set_style_text_color(hi_score_label, txt, 0);
    }

    if (game_speed < MAX_SPEED) {
        game_speed += SPEED_INCREMENT;
    }

    draw_game();

    if (frame_counter % 3 == 0) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%05d", score);
        lv_label_set_text(score_label, buf);
    }
}

static void key_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    if (game_state == STATE_IDLE) {
        if (key == LV_KEY_UP || key == ' ') {
            start_game();
            dino_vy = JUMP_VELOCITY;
            dino_jumping = true;
        }
        return;
    }

    if (game_state == STATE_GAMEOVER) {
        if (key == LV_KEY_ENTER || key == ' ' || key == LV_KEY_UP) {
            start_game();
        }
        return;
    }

    if (key == LV_KEY_UP || key == ' ') {
        if (!dino_jumping) {
            dino_vy = JUMP_VELOCITY;
            dino_jumping = true;
        }
    }

    if (key == LV_KEY_DOWN) {
        dino_ducking = true;
        dino_duck_frames = 20;
    }
}

static void init_clouds(void) {
    for (int i = 0; i < CLOUD_COUNT; i++) {
        clouds[i].x = 100.0f + i * 170.0f + (float)(rand() % 50);
        clouds[i].y = 40.0f + (float)(rand() % 100);
        clouds[i].w = 50 + rand() % 40;
        clouds[i].h = 14 + rand() % 8;
    }
}

static void init_ground_details(void) {
    for (int i = 0; i < 30; i++) {
        ground_details[i].x = rand() % SCREEN_W;
        ground_details[i].y = GROUND_Y + 4 + (rand() % 14);
        ground_details[i].w = 1 + rand() % 4;
    }
}

int main(void) {
    srand((unsigned)time(NULL));

    lv_init();

    lv_display_t *disp = lv_sdl_window_create(SCREEN_W, SCREEN_H);
    (void)disp;

    lv_indev_t *kb = lv_sdl_keyboard_create();
    lv_indev_t *mouse = lv_sdl_mouse_create();
    (void)mouse;

    scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0xF7F7F7), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    canvas = lv_canvas_create(scr);
    canvas_buf = malloc(SCREEN_W * SCREEN_H * 4);
    lv_canvas_set_buffer(canvas, canvas_buf, SCREEN_W, SCREEN_H, LV_COLOR_FORMAT_ARGB8888);
    lv_obj_set_pos(canvas, 0, 0);
    lv_obj_remove_flag(canvas, LV_OBJ_FLAG_SCROLLABLE);

    init_clouds();
    init_ground_details();

    for (int i = 0; i < MAX_OBSTACLES; i++) {
        obstacles[i].active = false;
    }

    score_label = lv_label_create(scr);
    lv_obj_set_style_text_font(score_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(score_label, lv_color_hex(0x535353), 0);
    lv_label_set_text(score_label, "00000");
    lv_obj_align(score_label, LV_ALIGN_TOP_RIGHT, -20, 15);

    hi_score_label = lv_label_create(scr);
    lv_obj_set_style_text_font(hi_score_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(hi_score_label, lv_color_hex(0x999999), 0);
    lv_label_set_text(hi_score_label, "HI 00000");
    lv_obj_align(hi_score_label, LV_ALIGN_TOP_RIGHT, -120, 20);

    gameover_label = lv_label_create(scr);
    lv_obj_set_style_text_font(gameover_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(gameover_label, lv_color_hex(0x535353), 0);
    lv_label_set_text(gameover_label, "G A M E  O V E R");
    lv_obj_align(gameover_label, LV_ALIGN_CENTER, 0, -40);
    lv_obj_add_flag(gameover_label, LV_OBJ_FLAG_HIDDEN);

    restart_hint = lv_label_create(scr);
    lv_obj_set_style_text_font(restart_hint, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(restart_hint, lv_color_hex(0x999999), 0);
    lv_label_set_text(restart_hint, "Press SPACE or ENTER to restart");
    lv_obj_align(restart_hint, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(restart_hint, LV_OBJ_FLAG_HIDDEN);

    start_hint = lv_label_create(scr);
    lv_obj_set_style_text_font(start_hint, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(start_hint, lv_color_hex(0x999999), 0);
    lv_label_set_text(start_hint, "Press SPACE or UP to start");
    lv_obj_align(start_hint, LV_ALIGN_CENTER, 0, 0);

    game_state = STATE_IDLE;
    dino_y = GROUND_Y;
    dino_vy = 0;
    dino_jumping = false;
    dino_ducking = false;
    dino_duck_frames = 0;
    hi_score = 0;
    night_mode = false;
    night_timer = 0;

    lv_obj_t *focus = lv_obj_create(scr);
    lv_obj_remove_style_all(focus);
    lv_obj_set_size(focus, 0, 0);
    lv_obj_add_flag(focus, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(focus, LV_OBJ_FLAG_CLICK_FOCUSABLE);

    lv_group_t *g = lv_group_create();
    lv_group_add_obj(g, focus);
    lv_indev_set_group(kb, g);
    lv_group_focus_obj(focus);

    lv_obj_add_event_cb(focus, key_event_cb, LV_EVENT_KEY, NULL);

    draw_game();

    game_timer = lv_timer_create(game_loop_cb, 16, NULL);

    while (1) {
        uint32_t idle = lv_timer_handler();
        if (idle > 0) {
            lv_delay_ms(idle < 5 ? idle : 5);
        }
    }

    return 0;
}
