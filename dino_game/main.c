/*
 * Chrome Dino (T-Rex Runner) game — pure LVGL widgets, no canvas.
 *
 * Every visual element is a pre-created lv_obj rectangle whose position,
 * size, color and visibility are updated each frame.
 */

#include "lvgl.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

/* ─── geometry ────────────────────────────────────────────────────── */

#define SCREEN_W  800
#define SCREEN_H  400
#define GROUND_Y  320

#define DINO_W     44
#define DINO_H     48
#define DINO_X     80
#define DINO_DUCK_H 30
#define DINO_DUCK_W 58

#define CACTUS_SMALL_W  17
#define CACTUS_SMALL_H  35
#define CACTUS_BIG_W    25
#define CACTUS_BIG_H    50

#define BIRD_W  46
#define BIRD_H  30

#define MAX_OBSTACLES    4
#define CLOUD_COUNT      5
#define GROUND_DOTS      30
#define STAR_COUNT       20

/* ─── physics ─────────────────────────────────────────────────────── */

#define JUMP_VELOCITY     (-14.0f)
#define GRAVITY           (0.65f)
#define FAST_FALL_GRAVITY (1.3f)

#define INITIAL_SPEED     6.0f
#define MAX_SPEED         14.0f
#define SPEED_INCREMENT   0.003f

#define OBSTACLE_MIN_GAP  250
#define OBSTACLE_MAX_GAP  450

/* ─── pre-allocated rect counts ───────────────────────────────────── */

#define DINO_RECTS       20
#define OBS_RECTS        16

/* ─── types ───────────────────────────────────────────────────────── */

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
    int w, h, y;
    bool active;
} obstacle_t;

typedef struct { float x, y; int w, h; } cloud_t;
typedef struct { int x, y, w; }          ground_detail_t;

typedef enum { STATE_IDLE, STATE_RUNNING, STATE_GAMEOVER } game_state_t;

/* ─── widget pools ────────────────────────────────────────────────── */

static lv_obj_t *scr;

static lv_obj_t *dino_rects[DINO_RECTS];
static lv_obj_t *obs_rects[MAX_OBSTACLES][OBS_RECTS];

static lv_obj_t *cloud_rects[CLOUD_COUNT][2];
static lv_obj_t *ground_line_obj;
static lv_obj_t *ground_dot_objs[GROUND_DOTS];
static lv_obj_t *star_objs[STAR_COUNT];

static lv_obj_t *score_label;
static lv_obj_t *hi_score_label;
static lv_obj_t *gameover_label;
static lv_obj_t *restart_hint;
static lv_obj_t *start_hint;

/* ─── game state ──────────────────────────────────────────────────── */

static obstacle_t obstacles[MAX_OBSTACLES];
static cloud_t    clouds[CLOUD_COUNT];
static ground_detail_t ground_details[GROUND_DOTS];
static int star_x[STAR_COUNT], star_y[STAR_COUNT];

static float dino_y, dino_vy;
static bool  dino_jumping, dino_ducking;
static int   dino_duck_frames;
static int   dino_anim_frame, dino_anim_counter;

static float game_speed;
static int   score, hi_score;
static game_state_t game_state;

static float next_obstacle_dist;
static int   frame_counter;
static bool  night_mode;
static int   night_timer;

static lv_timer_t *game_timer;

/* ─── colour helpers ──────────────────────────────────────────────── */

static lv_color_t bg_col(void)     { return night_mode ? lv_color_hex(0x1a1a2e) : lv_color_hex(0xF7F7F7); }
static lv_color_t fg_col(void)     { return night_mode ? lv_color_hex(0xE0E0E0) : lv_color_hex(0x535353); }
static lv_color_t gnd_col(void)    { return night_mode ? lv_color_hex(0x888888) : lv_color_hex(0x535353); }
static lv_color_t cld_col(void)    { return night_mode ? lv_color_hex(0x333355) : lv_color_hex(0xDDDDDD); }
static lv_color_t dot_col(void)    { return night_mode ? lv_color_hex(0x555555) : lv_color_hex(0xAAAAAA); }

/* ─── low-level rect helper ───────────────────────────────────────── */

static lv_obj_t *make_rect(lv_obj_t *parent) {
    lv_obj_t *r = lv_obj_create(parent);
    lv_obj_remove_style_all(r);
    lv_obj_set_style_radius(r, 0, 0);
    lv_obj_set_style_border_width(r, 0, 0);
    lv_obj_set_style_pad_all(r, 0, 0);
    lv_obj_set_style_bg_opa(r, LV_OPA_COVER, 0);
    lv_obj_remove_flag(r, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(r, LV_OBJ_FLAG_HIDDEN);
    return r;
}

static void show_rect(lv_obj_t *r, int x, int y, int w, int h, lv_color_t c) {
    lv_obj_set_pos(r, x, y);
    lv_obj_set_size(r, w, h);
    lv_obj_set_style_bg_color(r, c, 0);
    lv_obj_remove_flag(r, LV_OBJ_FLAG_HIDDEN);
}

static void hide_rect(lv_obj_t *r) {
    lv_obj_add_flag(r, LV_OBJ_FLAG_HIDDEN);
}

/* ─── sprite composers (fill pre-allocated rect arrays) ───────────── */

typedef struct { int x, y, w, h; bool white; } pixel_t;

static int compose_dino_standing(pixel_t *p) {
    int n = 0;
    p[n++] = (pixel_t){18,0,22,4,0};
    p[n++] = (pixel_t){14,4,30,4,0};
    p[n++] = (pixel_t){22,4,4,4,1};
    p[n++] = (pixel_t){10,8,30,4,0};
    p[n++] = (pixel_t){10,12,20,4,0};
    p[n++] = (pixel_t){6,16,12,4,0};
    p[n++] = (pixel_t){0,20,24,4,0};
    p[n++] = (pixel_t){6,24,14,4,0};
    p[n++] = (pixel_t){10,28,10,4,0};
    p[n++] = (pixel_t){10,32,14,4,0};
    p[n++] = (pixel_t){10,36,10,4,0};
    p[n++] = (pixel_t){10,40,4,8,0};
    p[n++] = (pixel_t){22,40,4,8,0};
    return n;
}

static int compose_dino_run(pixel_t *p, int frame) {
    int n = 0;
    p[n++] = (pixel_t){18,0,22,4,0};
    p[n++] = (pixel_t){14,4,30,4,0};
    p[n++] = (pixel_t){22,4,4,4,1};
    p[n++] = (pixel_t){10,8,30,4,0};
    p[n++] = (pixel_t){10,12,20,4,0};
    p[n++] = (pixel_t){6,16,12,4,0};
    p[n++] = (pixel_t){0,20,24,4,0};
    p[n++] = (pixel_t){6,24,14,4,0};
    p[n++] = (pixel_t){10,28,10,4,0};
    p[n++] = (pixel_t){10,32,14,4,0};
    p[n++] = (pixel_t){10,36,10,4,0};
    if (frame == 0) {
        p[n++] = (pixel_t){10,40,4,8,0};
        p[n++] = (pixel_t){22,40,4,4,0};
    } else {
        p[n++] = (pixel_t){10,40,4,4,0};
        p[n++] = (pixel_t){22,40,4,8,0};
    }
    return n;
}

static int compose_dino_duck(pixel_t *p, int frame) {
    int n = 0;
    p[n++] = (pixel_t){34,0,22,4,0};
    p[n++] = (pixel_t){30,4,28,4,0};
    p[n++] = (pixel_t){38,4,4,4,1};
    p[n++] = (pixel_t){0,8,54,4,0};
    p[n++] = (pixel_t){6,12,30,4,0};
    p[n++] = (pixel_t){10,16,22,4,0};
    p[n++] = (pixel_t){10,20,14,4,0};
    if (frame == 0) {
        p[n++] = (pixel_t){10,24,4,6,0};
        p[n++] = (pixel_t){20,24,4,2,0};
    } else {
        p[n++] = (pixel_t){10,24,4,2,0};
        p[n++] = (pixel_t){20,24,4,6,0};
    }
    return n;
}

static int compose_dino_dead(pixel_t *p) {
    int n = 0;
    p[n++] = (pixel_t){18,0,22,4,0};
    p[n++] = (pixel_t){14,4,30,4,0};
    p[n++] = (pixel_t){20,4,3,1,1};
    p[n++] = (pixel_t){21,5,1,1,1};
    p[n++] = (pixel_t){20,6,3,1,1};
    p[n++] = (pixel_t){10,8,30,4,0};
    p[n++] = (pixel_t){10,12,20,4,0};
    p[n++] = (pixel_t){6,16,12,4,0};
    p[n++] = (pixel_t){0,20,24,4,0};
    p[n++] = (pixel_t){6,24,14,4,0};
    p[n++] = (pixel_t){10,28,10,4,0};
    p[n++] = (pixel_t){10,32,14,4,0};
    p[n++] = (pixel_t){10,36,10,4,0};
    p[n++] = (pixel_t){10,40,4,8,0};
    p[n++] = (pixel_t){22,40,4,8,0};
    return n;
}

static void apply_dino(pixel_t *pix, int cnt, int ox, int oy) {
    lv_color_t dk = fg_col();
    for (int i = 0; i < DINO_RECTS; i++) {
        if (i < cnt)
            show_rect(dino_rects[i], ox + pix[i].x, oy + pix[i].y,
                      pix[i].w, pix[i].h,
                      pix[i].white ? lv_color_white() : dk);
        else
            hide_rect(dino_rects[i]);
    }
}

/* obstacle sprite composers */

static int compose_cactus_small(pixel_t *p) {
    int n = 0;
    p[n++] = (pixel_t){5,0,7,20,0};
    p[n++] = (pixel_t){0,7,5,10,0};
    p[n++] = (pixel_t){12,5,5,10,0};
    p[n++] = (pixel_t){5,20,7,15,0};
    return n;
}

static int compose_cactus_big(pixel_t *p) {
    int n = 0;
    p[n++] = (pixel_t){8,0,9,30,0};
    p[n++] = (pixel_t){0,10,8,12,0};
    p[n++] = (pixel_t){17,6,8,12,0};
    p[n++] = (pixel_t){8,30,9,20,0};
    return n;
}

static int compose_cactus_cluster(pixel_t *p) {
    int n = 0;
    for (int i = 0; i < 3; i++) {
        int ox = i * CACTUS_SMALL_W;
        int oy = (i == 1) ? 0 : 5;
        p[n++] = (pixel_t){ox+5, oy,    7, 20, 0};
        p[n++] = (pixel_t){ox,   oy+7,  5, 10, 0};
        p[n++] = (pixel_t){ox+12,oy+5,  5, 10, 0};
        p[n++] = (pixel_t){ox+5, oy+20, 7, 15, 0};
    }
    return n;
}

static int compose_bird(pixel_t *p, int frame) {
    int n = 0;
    p[n++] = (pixel_t){4,14,38,6,0};
    p[n++] = (pixel_t){0,12,8,8,0};
    if (frame == 0) {
        p[n++] = (pixel_t){10,6,6,8,0};
        p[n++] = (pixel_t){16,2,6,6,0};
    } else {
        p[n++] = (pixel_t){10,20,6,8,0};
        p[n++] = (pixel_t){16,24,6,6,0};
    }
    return n;
}

static void apply_obstacle(int idx, pixel_t *pix, int cnt, int ox, int oy) {
    lv_color_t c = fg_col();
    for (int i = 0; i < OBS_RECTS; i++) {
        if (i < cnt)
            show_rect(obs_rects[idx][i], ox + pix[i].x, oy + pix[i].y,
                      pix[i].w, pix[i].h, c);
        else
            hide_rect(obs_rects[idx][i]);
    }
}

/* ─── visual update ───────────────────────────────────────────────── */

static void update_visuals(void) {
    lv_color_t bg = bg_col();
    lv_obj_set_style_bg_color(scr, bg, 0);

    /* clouds */
    lv_color_t cc = cld_col();
    for (int i = 0; i < CLOUD_COUNT; i++) {
        int cx = (int)clouds[i].x;
        int cy = (int)clouds[i].y;
        show_rect(cloud_rects[i][0], cx, cy + 4, clouds[i].w, clouds[i].h - 8, cc);
        show_rect(cloud_rects[i][1], cx + 10, cy, clouds[i].w - 20, clouds[i].h, cc);
    }

    /* stars */
    if (night_mode) {
        lv_color_t sc = lv_color_hex(0xCCCCCC);
        for (int i = 0; i < STAR_COUNT; i++)
            show_rect(star_objs[i], star_x[i], star_y[i], 2, 2, sc);
    } else {
        for (int i = 0; i < STAR_COUNT; i++)
            hide_rect(star_objs[i]);
    }

    /* ground line */
    show_rect(ground_line_obj, 0, GROUND_Y, SCREEN_W, 2, gnd_col());

    /* ground dots (scroll) */
    lv_color_t dc = dot_col();
    for (int i = 0; i < GROUND_DOTS; i++) {
        int gx = ground_details[i].x - (int)(game_speed * frame_counter * 0.5f) % SCREEN_W;
        while (gx < 0) gx += SCREEN_W;
        gx %= SCREEN_W;
        show_rect(ground_dot_objs[i], gx, ground_details[i].y, ground_details[i].w, 1, dc);
    }

    /* dino */
    pixel_t pix[DINO_RECTS];
    int cnt;
    int dh;

    if (game_state == STATE_GAMEOVER) {
        cnt = compose_dino_dead(pix);
        dh = DINO_H;
    } else if (dino_ducking && !dino_jumping) {
        cnt = compose_dino_duck(pix, dino_anim_frame);
        dh = DINO_DUCK_H;
    } else if (game_state == STATE_RUNNING && !dino_jumping) {
        cnt = compose_dino_run(pix, dino_anim_frame);
        dh = DINO_H;
    } else {
        cnt = compose_dino_standing(pix);
        dh = DINO_H;
    }
    apply_dino(pix, cnt, DINO_X, (int)dino_y - dh);

    /* obstacles */
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!obstacles[i].active) {
            for (int j = 0; j < OBS_RECTS; j++) hide_rect(obs_rects[i][j]);
            continue;
        }
        obstacle_t *obs = &obstacles[i];
        pixel_t op[OBS_RECTS];
        int oc = 0;
        switch (obs->type) {
            case OBS_CACTUS_SMALL:   oc = compose_cactus_small(op);          break;
            case OBS_CACTUS_BIG:     oc = compose_cactus_big(op);            break;
            case OBS_CACTUS_CLUSTER: oc = compose_cactus_cluster(op);        break;
            case OBS_BIRD_LOW:
            case OBS_BIRD_HIGH:      oc = compose_bird(op, dino_anim_frame); break;
            default: break;
        }
        apply_obstacle(i, op, oc, (int)obs->x, obs->y);
    }
}

/* ─── game logic ──────────────────────────────────────────────────── */

static void spawn_obstacle(void) {
    int slot = -1;
    for (int i = 0; i < MAX_OBSTACLES; i++)
        if (!obstacles[i].active) { slot = i; break; }
    if (slot < 0) return;

    obstacle_t *obs = &obstacles[slot];
    obs->active = true;
    obs->x = SCREEN_W + 20;

    int r = rand() % 100;
    if (score > 200 && r < 20) {
        if (rand() % 2) {
            obs->type = OBS_BIRD_HIGH;
            obs->w = BIRD_W; obs->h = BIRD_H;
            obs->y = GROUND_Y - 80;
        } else {
            obs->type = OBS_BIRD_LOW;
            obs->w = BIRD_W; obs->h = BIRD_H;
            obs->y = GROUND_Y - BIRD_H - 5;
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
        obs->w = CACTUS_SMALL_W * 3;
        obs->h = CACTUS_SMALL_H + 5;
        obs->y = GROUND_Y - obs->h;
    }
}

static bool check_collision(void) {
    int dcw, dch, dcy;
    if (dino_ducking && !dino_jumping) {
        dcw = DINO_DUCK_W - 10; dch = DINO_DUCK_H - 4;
        dcy = (int)dino_y - DINO_DUCK_H;
    } else {
        dcw = DINO_W - 10; dch = DINO_H - 8;
        dcy = (int)dino_y - DINO_H;
    }
    int dl = DINO_X + 5, dr = DINO_X + dcw;
    int dt = dcy + 4,    db = dcy + dch;

    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!obstacles[i].active) continue;
        obstacle_t *o = &obstacles[i];
        int ol = (int)o->x + 3,  or_ = (int)o->x + o->w - 3;
        int ot = o->y + 3,       ob  = o->y + o->h - 3;
        if (dl < or_ && dr > ol && dt < ob && db > ot) return true;
    }
    return false;
}

static void start_game(void) {
    game_state = STATE_RUNNING;
    score = 0;
    game_speed = INITIAL_SPEED;
    dino_y = GROUND_Y; dino_vy = 0;
    dino_jumping = false; dino_ducking = false;
    dino_duck_frames = 0;
    dino_anim_frame = 0; dino_anim_counter = 0;
    frame_counter = 0;
    next_obstacle_dist = 300;
    night_mode = false; night_timer = 0;

    for (int i = 0; i < MAX_OBSTACLES; i++) obstacles[i].active = false;

    lv_obj_add_flag(gameover_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(restart_hint,   LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(start_hint,     LV_OBJ_FLAG_HIDDEN);

    lv_label_set_text(score_label, "00000");
}

static void game_over(void) {
    game_state = STATE_GAMEOVER;
    if (score > hi_score) hi_score = score;
    char buf[32];
    snprintf(buf, sizeof(buf), "HI %05d", hi_score);
    lv_label_set_text(hi_score_label, buf);
    lv_obj_remove_flag(gameover_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(restart_hint,   LV_OBJ_FLAG_HIDDEN);
}

static void game_loop_cb(lv_timer_t *t) {
    (void)t;

    if (game_state != STATE_RUNNING) { update_visuals(); return; }

    frame_counter++;

    /* physics */
    if (dino_jumping) {
        dino_vy += dino_ducking ? FAST_FALL_GRAVITY : GRAVITY;
        dino_y  += dino_vy;
        if (dino_y >= GROUND_Y) { dino_y = GROUND_Y; dino_vy = 0; dino_jumping = false; }
    }
    if (dino_duck_frames > 0) { dino_duck_frames--; if (!dino_duck_frames) dino_ducking = false; }

    dino_anim_counter++;
    if (dino_anim_counter >= 6) { dino_anim_counter = 0; dino_anim_frame ^= 1; }

    /* move obstacles */
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!obstacles[i].active) continue;
        obstacles[i].x -= game_speed;
        if (obstacles[i].x < -100) obstacles[i].active = false;
    }

    /* spawn */
    next_obstacle_dist -= game_speed;
    if (next_obstacle_dist <= 0) {
        spawn_obstacle();
        next_obstacle_dist = OBSTACLE_MIN_GAP + (rand() % (OBSTACLE_MAX_GAP - OBSTACLE_MIN_GAP));
        next_obstacle_dist *= game_speed / INITIAL_SPEED;
    }

    /* clouds drift */
    for (int i = 0; i < CLOUD_COUNT; i++) {
        clouds[i].x -= game_speed * 0.3f;
        if (clouds[i].x + clouds[i].w < 0) {
            clouds[i].x = SCREEN_W + rand() % 100;
            clouds[i].y = 40 + rand() % 100;
        }
    }

    /* collision */
    if (check_collision()) { game_over(); update_visuals(); return; }

    score++;

    /* day/night */
    night_timer++;
    if (night_timer >= 700) {
        night_mode = !night_mode;
        night_timer = 0;
        lv_color_t txt = night_mode ? lv_color_hex(0xE0E0E0) : lv_color_hex(0x535353);
        lv_obj_set_style_text_color(score_label,    txt, 0);
        lv_obj_set_style_text_color(hi_score_label, txt, 0);
        lv_obj_set_style_text_color(gameover_label, txt, 0);
    }

    if (game_speed < MAX_SPEED) game_speed += SPEED_INCREMENT;

    update_visuals();

    if (frame_counter % 3 == 0) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%05d", score);
        lv_label_set_text(score_label, buf);
    }
}

/* ─── input ───────────────────────────────────────────────────────── */

static void key_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    uint32_t k = lv_event_get_key(e);

    if (game_state == STATE_IDLE) {
        if (k == LV_KEY_UP || k == ' ') {
            start_game();
            dino_vy = JUMP_VELOCITY; dino_jumping = true;
        }
        return;
    }
    if (game_state == STATE_GAMEOVER) {
        if (k == LV_KEY_ENTER || k == ' ' || k == LV_KEY_UP) start_game();
        return;
    }
    if (k == LV_KEY_UP || k == ' ') {
        if (!dino_jumping) { dino_vy = JUMP_VELOCITY; dino_jumping = true; }
    }
    if (k == LV_KEY_DOWN) { dino_ducking = true; dino_duck_frames = 20; }
}

/* ─── init helpers ────────────────────────────────────────────────── */

static void init_clouds(void) {
    for (int i = 0; i < CLOUD_COUNT; i++) {
        clouds[i].x = 100.0f + i * 170.0f + (float)(rand() % 50);
        clouds[i].y = 40.0f  + (float)(rand() % 100);
        clouds[i].w = 50 + rand() % 40;
        clouds[i].h = 14 + rand() % 8;
    }
}

static void init_ground(void) {
    for (int i = 0; i < GROUND_DOTS; i++) {
        ground_details[i].x = rand() % SCREEN_W;
        ground_details[i].y = GROUND_Y + 4 + (rand() % 14);
        ground_details[i].w = 1 + rand() % 4;
    }
}

static void init_stars(void) {
    srand(42);
    for (int i = 0; i < STAR_COUNT; i++) {
        star_x[i] = rand() % SCREEN_W;
        star_y[i] = 20 + rand() % 150;
    }
    srand((unsigned)time(NULL));
}

/* ─── main ────────────────────────────────────────────────────────── */

int main(void) {
    srand((unsigned)time(NULL));
    lv_init();

    lv_display_t *disp = lv_sdl_window_create(SCREEN_W, SCREEN_H);
    (void)disp;
    lv_indev_t *kb    = lv_sdl_keyboard_create();
    lv_indev_t *mouse = lv_sdl_mouse_create();
    (void)mouse;

    scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0xF7F7F7), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    /* ── pre-create all rect pools ── */

    for (int i = 0; i < CLOUD_COUNT; i++)
        for (int j = 0; j < 2; j++)
            cloud_rects[i][j] = make_rect(scr);

    for (int i = 0; i < STAR_COUNT; i++)
        star_objs[i] = make_rect(scr);

    ground_line_obj = make_rect(scr);

    for (int i = 0; i < GROUND_DOTS; i++)
        ground_dot_objs[i] = make_rect(scr);

    for (int i = 0; i < DINO_RECTS; i++)
        dino_rects[i] = make_rect(scr);

    for (int i = 0; i < MAX_OBSTACLES; i++)
        for (int j = 0; j < OBS_RECTS; j++)
            obs_rects[i][j] = make_rect(scr);

    /* ── labels ── */

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

    /* ── keyboard focus ── */

    lv_obj_t *focus = lv_obj_create(scr);
    lv_obj_remove_style_all(focus);
    lv_obj_set_size(focus, 0, 0);
    lv_obj_add_flag(focus, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE);

    lv_group_t *g = lv_group_create();
    lv_group_add_obj(g, focus);
    lv_indev_set_group(kb, g);
    lv_group_focus_obj(focus);
    lv_obj_add_event_cb(focus, key_event_cb, LV_EVENT_KEY, NULL);

    /* ── init data ── */

    init_clouds();
    init_ground();
    init_stars();
    for (int i = 0; i < MAX_OBSTACLES; i++) obstacles[i].active = false;

    game_state = STATE_IDLE;
    dino_y = GROUND_Y; dino_vy = 0;
    dino_jumping = false; dino_ducking = false;
    dino_duck_frames = 0; hi_score = 0;
    night_mode = false; night_timer = 0;

    update_visuals();

    game_timer = lv_timer_create(game_loop_cb, 16, NULL);

    while (1) {
        uint32_t idle = lv_timer_handler();
        if (idle > 0) lv_delay_ms(idle < 5 ? idle : 5);
    }
    return 0;
}
