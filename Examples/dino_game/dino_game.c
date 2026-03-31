/**
 * @file dino_game.c
 * @brief Chrome-style T-Rex runner game for GUI_GSP (128x64 monochrome display).
 *
 * Implements:
 *  - T-Rex sprite with running animation (2 frames) and ducking
 *  - Gravity-based jump
 *  - Scrolling ground with dashed pattern
 *  - Three obstacle types: small cactus, large cactus, cactus group, bird
 *  - Collision detection (AABB)
 *  - Score / high-score display
 *  - Day/night palette inversion
 *  - Game-over splash with blinking "GAME OVER" text
 *
 * The game uses a GSP Timer (USED_TIMER) for the update tick and draws
 * directly via GSP_Driver primitives (PutPixel, Bar, Line, SetColor, etc.).
 */

#include "dino_game.h"
#include "GSP_DrawText.h"
#include <string.h>

/* ================================================================
 *  Pixel-art sprite data  (1 = ink, 0 = transparent)
 *  Stored as arrays of bytes, 1 bit per pixel, MSB-left.
 * ================================================================ */

/* T-Rex standing / run frame 1  (20 wide x 22 tall) */
static const unsigned char dino_run1[] = {
    /*          col: 0         8        16   19 */
    /* row  0 */  0x00, 0x1F, 0xC0,
    /* row  1 */  0x00, 0x3F, 0xE0,
    /* row  2 */  0x00, 0x37, 0xE0,
    /* row  3 */  0x00, 0x3F, 0xE0,
    /* row  4 */  0x00, 0x3F, 0xE0,
    /* row  5 */  0x00, 0x3F, 0xE0,
    /* row  6 */  0x00, 0x3E, 0x00,
    /* row  7 */  0x00, 0x3F, 0x80,
    /* row  8 */  0x20, 0x7F, 0x00,
    /* row  9 */  0x30, 0xFF, 0x00,
    /* row 10 */  0x79, 0xFF, 0x00,
    /* row 11 */  0xFF, 0xFF, 0x00,
    /* row 12 */  0xFF, 0xFE, 0x00,
    /* row 13 */  0x7F, 0xFE, 0x00,
    /* row 14 */  0x3F, 0xFC, 0x00,
    /* row 15 */  0x1F, 0xFC, 0x00,
    /* row 16 */  0x0F, 0xF8, 0x00,
    /* row 17 */  0x07, 0xFC, 0x00,
    /* row 18 */  0x01, 0xEC, 0x00,
    /* row 19 */  0x01, 0xCC, 0x00,
    /* row 20 */  0x01, 0x84, 0x00,
    /* row 21 */  0x01, 0x06, 0x00,
};
#define DINO_RUN1_W 20
#define DINO_RUN1_H 22

/* T-Rex run frame 2 — same body, different leg position */
static const unsigned char dino_run2[] = {
    0x00, 0x1F, 0xC0,
    0x00, 0x3F, 0xE0,
    0x00, 0x37, 0xE0,
    0x00, 0x3F, 0xE0,
    0x00, 0x3F, 0xE0,
    0x00, 0x3F, 0xE0,
    0x00, 0x3E, 0x00,
    0x00, 0x3F, 0x80,
    0x20, 0x7F, 0x00,
    0x30, 0xFF, 0x00,
    0x79, 0xFF, 0x00,
    0xFF, 0xFF, 0x00,
    0xFF, 0xFE, 0x00,
    0x7F, 0xFE, 0x00,
    0x3F, 0xFC, 0x00,
    0x1F, 0xFC, 0x00,
    0x0F, 0xF8, 0x00,
    0x07, 0xF0, 0x00,
    0x03, 0xE0, 0x00,
    0x01, 0xC0, 0x00,
    0x01, 0xC0, 0x00,
    0x03, 0xE0, 0x00,
};
#define DINO_RUN2_W 20
#define DINO_RUN2_H 22

/* T-Rex ducking frame  (26 wide x 14 tall) */
static const unsigned char dino_duck[] = {
    0x00, 0x00, 0x1F, 0xC0,
    0x00, 0x00, 0x3F, 0xE0,
    0x00, 0x00, 0x37, 0xE0,
    0x00, 0x00, 0x3F, 0xE0,
    0x00, 0x00, 0x3F, 0xE0,
    0x00, 0x00, 0x3E, 0x00,
    0x20, 0x00, 0x7F, 0x80,
    0x70, 0x01, 0xFF, 0x00,
    0xFC, 0x7F, 0xFF, 0x00,
    0xFF, 0xFF, 0xFE, 0x00,
    0x7F, 0xFF, 0xFC, 0x00,
    0x1F, 0xFF, 0xF8, 0x00,
    0x03, 0xCE, 0x00, 0x00,
    0x02, 0x04, 0x00, 0x00,
};
#define DINO_DUCK_W 26
#define DINO_DUCK_H 14

/* Small cactus (7 wide x 14 tall) */
static const unsigned char cactus_small[] = {
    0x10,
    0x10,
    0x50,
    0xD0,
    0xD6,
    0xD6,
    0xD6,
    0xFE,
    0x7C,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
};
#define CACTUS_SM_W 7
#define CACTUS_SM_H 14

/* Large cactus (11 wide x 20 tall) */
static const unsigned char cactus_large[] = {
    0x04, 0x00,
    0x04, 0x00,
    0x04, 0x00,
    0x44, 0x40,
    0xE4, 0xE0,
    0xE4, 0xE0,
    0xE4, 0xE0,
    0xE4, 0xE0,
    0xFC, 0xE0,
    0x7F, 0xE0,
    0x1F, 0xC0,
    0x07, 0x00,
    0x04, 0x00,
    0x04, 0x00,
    0x04, 0x00,
    0x04, 0x00,
    0x04, 0x00,
    0x04, 0x00,
    0x04, 0x00,
    0x04, 0x00,
};
#define CACTUS_LG_W 11
#define CACTUS_LG_H 20

/* Bird / pterodactyl frame 1 (18 wide x 10 tall) */
static const unsigned char bird_frame1[] = {
    0x04, 0x00, 0x00,
    0x0E, 0x00, 0x00,
    0x1F, 0x00, 0x00,
    0x1F, 0x00, 0x00,
    0xFF, 0xFF, 0xC0,
    0x7F, 0xFF, 0xC0,
    0x00, 0x7F, 0xC0,
    0x00, 0x3E, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
};
#define BIRD_F1_W 18
#define BIRD_F1_H 10

/* Bird / pterodactyl frame 2 (18 wide x 10 tall) */
static const unsigned char bird_frame2[] = {
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xC0,
    0x7F, 0xFF, 0xC0,
    0x00, 0x7F, 0xC0,
    0x00, 0x3E, 0x00,
    0x1F, 0x00, 0x00,
    0x0E, 0x00, 0x00,
};
#define BIRD_F2_W 18
#define BIRD_F2_H 10

/* "GAME OVER" splash text — drawn with DrawTextUTF8 using the default font */

/* ================================================================
 *  Helpers
 * ================================================================ */

static DinoGame game;

static unsigned int dino_rand(DinoGame *g)
{
    g->rng ^= g->rng << 13;
    g->rng ^= g->rng >> 17;
    g->rng ^= g->rng << 5;
    return g->rng;
}

static short rand_range(DinoGame *g, short lo, short hi)
{
    if (lo >= hi) return lo;
    return lo + (short)(dino_rand(g) % (unsigned int)(hi - lo + 1));
}

/* Draw a 1-bit sprite. Ink pixels use current _color, transparent pixels are skipped. */
static void draw_sprite(short sx, short sy, const unsigned char *data,
                        short w, short h, short bytes_per_row)
{
    for (short row = 0; row < h; row++) {
        const unsigned char *rowp = data + row * bytes_per_row;
        for (short col = 0; col < w; col++) {
            int byte_idx = col >> 3;
            int bit_idx  = 7 - (col & 7);
            if (rowp[byte_idx] & (1 << bit_idx)) {
                PutPixel(sx + col, sy + row);
            }
        }
    }
}

/* ================================================================
 *  Obstacle management
 * ================================================================ */

static void obs_init(DinoObstacle *o)
{
    o->type = OBS_NONE;
    o->x = 0;
    o->y = 0;
    o->w = 0;
    o->h = 0;
    o->frame = 0;
}

static void obs_spawn(DinoGame *g, DinoObstacle *o)
{
    short t = rand_range(g, 0, 100);

    if (g->score > 200 && t > 75) {
        o->type = OBS_BIRD;
        o->w = BIRD_F1_W;
        o->h = BIRD_F1_H;
        short bird_heights[] = {DINO_GROUND_Y - 22, DINO_GROUND_Y - 14, DINO_GROUND_Y - 8};
        o->y = bird_heights[rand_range(g, 0, 2)];
    } else if (t > 55) {
        o->type = OBS_CACTUS_GROUP;
        o->w = CACTUS_SM_W * 2 + 2;
        o->h = CACTUS_SM_H;
        o->y = DINO_GROUND_Y - o->h;
    } else if (t > 30) {
        o->type = OBS_CACTUS_LARGE;
        o->w = CACTUS_LG_W;
        o->h = CACTUS_LG_H;
        o->y = DINO_GROUND_Y - o->h;
    } else {
        o->type = OBS_CACTUS_SMALL;
        o->w = CACTUS_SM_W;
        o->h = CACTUS_SM_H;
        o->y = DINO_GROUND_Y - o->h;
    }

    o->x = SCREEN_HOR_SIZE + rand_range(g, 0, 20);
    o->frame = 0;
}

/* ================================================================
 *  Game reset
 * ================================================================ */

void DinoGame_Reset(DinoGame *g)
{
    g->state = DINO_STATE_RUNNING;
    g->dino_x = 10;
    g->dino_y = DINO_GROUND_Y - DINO_RUN1_H;
    g->dino_vy = 0;
    g->dino_ducking = false;
    g->dino_w = DINO_RUN1_W;
    g->dino_h = DINO_RUN1_H;
    g->anim_frame = 0;
    g->anim_timer = 0;

    for (int i = 0; i < DINO_MAX_OBSTACLES; i++)
        obs_init(&g->obs[i]);

    g->next_obs_dist = DINO_MIN_OBS_GAP + 30;
    g->ground_offset = 0;
    g->score = 0;
    g->speed = DINO_START_SPEED;
    g->night = false;
    g->night_timer = 0;
    g->blink = false;
    g->blink_timer = 0;

    g->cloud_x[0] = 20;  g->cloud_y[0] = 8;
    g->cloud_x[1] = 80;  g->cloud_y[1] = 14;

    if (g->rng == 0) g->rng = 42;
}

/* ================================================================
 *  Physics / logic tick  (called from timer callback)
 * ================================================================ */

static void game_tick(DinoGame *g, int key)
{
    if (g->state == DINO_STATE_IDLE) {
        if (key == DINO_KEY_JUMP || key == DINO_KEY_OK)
            DinoGame_Reset(g);
        return;
    }

    if (g->state == DINO_STATE_GAME_OVER) {
        g->blink_timer++;
        if (g->blink_timer >= 6) {
            g->blink_timer = 0;
            g->blink = !g->blink;
        }
        if (key == DINO_KEY_JUMP || key == DINO_KEY_OK)
            DinoGame_Reset(g);
        return;
    }

    /* --- Running state --- */

    /* Jump */
    bool on_ground = (g->dino_y >= DINO_GROUND_Y - g->dino_h);
    if ((key == DINO_KEY_JUMP || key == DINO_KEY_OK) && on_ground && !g->dino_ducking) {
        g->dino_vy = DINO_JUMP_VELOCITY;
    }

    /* Duck */
    if (key == DINO_KEY_DUCK && on_ground) {
        g->dino_ducking = true;
        g->dino_w = DINO_DUCK_W;
        g->dino_h = DINO_DUCK_H;
        g->dino_y = DINO_GROUND_Y - DINO_DUCK_H;
    } else {
        if (g->dino_ducking) {
            g->dino_ducking = false;
            g->dino_w = DINO_RUN1_W;
            g->dino_h = DINO_RUN1_H;
            if (on_ground)
                g->dino_y = DINO_GROUND_Y - DINO_RUN1_H;
        }
    }

    /* Gravity */
    g->dino_vy += DINO_GRAVITY;
    if (g->dino_vy > DINO_MAX_FALL)
        g->dino_vy = DINO_MAX_FALL;
    g->dino_y += g->dino_vy;

    short floor_y = DINO_GROUND_Y - g->dino_h;
    if (g->dino_y >= floor_y) {
        g->dino_y = floor_y;
        g->dino_vy = 0;
    }

    /* Animate legs */
    g->anim_timer++;
    if (g->anim_timer >= 4) {
        g->anim_timer = 0;
        g->anim_frame ^= 1;
    }

    /* Move obstacles */
    for (int i = 0; i < DINO_MAX_OBSTACLES; i++) {
        DinoObstacle *o = &g->obs[i];
        if (o->type == OBS_NONE) continue;
        o->x -= g->speed;
        if (o->type == OBS_BIRD) {
            o->frame++;
            if (o->frame >= 8) o->frame = 0;
        }
        if (o->x + o->w < -5) {
            o->type = OBS_NONE;
        }
    }

    /* Spawn obstacles */
    g->next_obs_dist -= g->speed;
    if (g->next_obs_dist <= 0) {
        for (int i = 0; i < DINO_MAX_OBSTACLES; i++) {
            if (g->obs[i].type == OBS_NONE) {
                obs_spawn(g, &g->obs[i]);
                g->next_obs_dist = rand_range(g, DINO_MIN_OBS_GAP, DINO_MAX_OBS_GAP);
                break;
            }
        }
    }

    /* Move clouds */
    for (int i = 0; i < 2; i++) {
        g->cloud_x[i] -= 1;
        if (g->cloud_x[i] < -20) {
            g->cloud_x[i] = SCREEN_HOR_SIZE + rand_range(g, 0, 30);
            g->cloud_y[i] = rand_range(g, 5, 20);
        }
    }

    /* Scroll ground */
    g->ground_offset = (g->ground_offset + g->speed) % 12;

    /* Score */
    g->score++;
    if (g->score > g->hi_score)
        g->hi_score = g->score;

    /* Speed up */
    unsigned char new_speed = DINO_START_SPEED + (unsigned char)(g->score / DINO_SPEED_INC_SCORE);
    if (new_speed > DINO_MAX_SPEED) new_speed = DINO_MAX_SPEED;
    g->speed = new_speed;

    /* Night toggle every ~700 ticks */
    g->night_timer++;
    if (g->night_timer >= 700) {
        g->night_timer = 0;
        g->night = !g->night;
    }

    /* Collision detection (shrink hitboxes for fairness) */
    short dx_margin = 3, dy_margin = 3;
    short dino_left   = g->dino_x + dx_margin;
    short dino_right  = g->dino_x + g->dino_w - dx_margin;
    short dino_top    = g->dino_y + dy_margin;
    short dino_bottom = g->dino_y + g->dino_h - dy_margin;

    for (int i = 0; i < DINO_MAX_OBSTACLES; i++) {
        DinoObstacle *o = &g->obs[i];
        if (o->type == OBS_NONE) continue;
        short ol = o->x + 1;
        short or_ = o->x + o->w - 1;
        short ot = o->y + 1;
        short ob = o->y + o->h - 1;
        if (dino_right > ol && dino_left < or_ && dino_bottom > ot && dino_top < ob) {
            g->state = DINO_STATE_GAME_OVER;
            g->blink = true;
            g->blink_timer = 0;
            return;
        }
    }
}

/* ================================================================
 *  Rendering
 * ================================================================ */

static void draw_ground(DinoGame *g)
{
    WORD_COLOR ink   = g->night ? WHITE : BLACK;
    WORD_COLOR paper = g->night ? BLACK : WHITE;
    (void)paper;

    SetColor(ink);
    Line(0, DINO_HORIZON_Y, SCREEN_HOR_SIZE - 1, DINO_HORIZON_Y);

    for (short x = -g->ground_offset; x < SCREEN_HOR_SIZE; x += 12) {
        short x1 = x;
        short x2 = x + 5;
        if (x1 < 0) x1 = 0;
        if (x2 >= SCREEN_HOR_SIZE) x2 = SCREEN_HOR_SIZE - 1;
        if (x1 < x2)
            Line(x1, DINO_HORIZON_Y + 3, x2, DINO_HORIZON_Y + 3);
    }
    for (short x = 6 - g->ground_offset; x < SCREEN_HOR_SIZE; x += 18) {
        if (x >= 0 && x < SCREEN_HOR_SIZE)
            PutPixel(x, DINO_HORIZON_Y + 5);
        if (x + 3 >= 0 && x + 3 < SCREEN_HOR_SIZE)
            PutPixel(x + 3, DINO_HORIZON_Y + 6);
    }
}

static void draw_cloud(short cx, short cy, WORD_COLOR ink)
{
    SetColor(ink);
    Line(cx + 2, cy,     cx + 8, cy);
    Line(cx + 1, cy + 1, cx + 11, cy + 1);
    Line(cx,     cy + 2, cx + 12, cy + 2);
    Line(cx + 1, cy + 3, cx + 11, cy + 3);
    Line(cx + 3, cy + 4, cx + 9, cy + 4);
}

static void draw_dino(DinoGame *g)
{
    WORD_COLOR ink = g->night ? WHITE : BLACK;
    SetColor(ink);

    if (g->dino_ducking) {
        draw_sprite(g->dino_x, g->dino_y, dino_duck, DINO_DUCK_W, DINO_DUCK_H, 4);
    } else {
        bool in_air = (g->dino_y < (DINO_GROUND_Y - DINO_RUN1_H));
        if (in_air || g->state != DINO_STATE_RUNNING) {
            draw_sprite(g->dino_x, g->dino_y, dino_run1, DINO_RUN1_W, DINO_RUN1_H, 3);
        } else {
            if (g->anim_frame == 0)
                draw_sprite(g->dino_x, g->dino_y, dino_run1, DINO_RUN1_W, DINO_RUN1_H, 3);
            else
                draw_sprite(g->dino_x, g->dino_y, dino_run2, DINO_RUN2_W, DINO_RUN2_H, 3);
        }
    }
}

static void draw_obstacles(DinoGame *g)
{
    WORD_COLOR ink = g->night ? WHITE : BLACK;
    SetColor(ink);

    for (int i = 0; i < DINO_MAX_OBSTACLES; i++) {
        DinoObstacle *o = &g->obs[i];
        if (o->type == OBS_NONE) continue;

        switch (o->type) {
        case OBS_CACTUS_SMALL:
            draw_sprite(o->x, o->y, cactus_small, CACTUS_SM_W, CACTUS_SM_H, 1);
            break;
        case OBS_CACTUS_LARGE:
            draw_sprite(o->x, o->y, cactus_large, CACTUS_LG_W, CACTUS_LG_H, 2);
            break;
        case OBS_CACTUS_GROUP:
            draw_sprite(o->x, o->y, cactus_small, CACTUS_SM_W, CACTUS_SM_H, 1);
            draw_sprite(o->x + CACTUS_SM_W + 2, o->y, cactus_small, CACTUS_SM_W, CACTUS_SM_H, 1);
            break;
        case OBS_BIRD:
            if (o->frame < 4)
                draw_sprite(o->x, o->y, bird_frame1, BIRD_F1_W, BIRD_F1_H, 3);
            else
                draw_sprite(o->x, o->y, bird_frame2, BIRD_F2_W, BIRD_F2_H, 3);
            break;
        default:
            break;
        }
    }
}

static void draw_score(DinoGame *g)
{
    char buf[24];
    WORD_COLOR ink = g->night ? WHITE : BLACK;

    if (g->hi_score > 0) {
        /* "HI 00000  00000" */
        int hi_val = (int)(g->hi_score / 3);
        int sc_val = (int)(g->score / 3);
        int len = 0;
        buf[len++] = 'H';
        buf[len++] = 'I';
        buf[len++] = ' ';
        buf[len++] = '0' + (hi_val / 10000) % 10;
        buf[len++] = '0' + (hi_val / 1000) % 10;
        buf[len++] = '0' + (hi_val / 100) % 10;
        buf[len++] = '0' + (hi_val / 10) % 10;
        buf[len++] = '0' + hi_val % 10;
        buf[len++] = ' ';
        buf[len++] = ' ';
        buf[len++] = '0' + (sc_val / 10000) % 10;
        buf[len++] = '0' + (sc_val / 1000) % 10;
        buf[len++] = '0' + (sc_val / 100) % 10;
        buf[len++] = '0' + (sc_val / 10) % 10;
        buf[len++] = '0' + sc_val % 10;
        buf[len] = '\0';
    } else {
        int sc_val = (int)(g->score / 3);
        int len = 0;
        buf[len++] = '0' + (sc_val / 10000) % 10;
        buf[len++] = '0' + (sc_val / 1000) % 10;
        buf[len++] = '0' + (sc_val / 100) % 10;
        buf[len++] = '0' + (sc_val / 10) % 10;
        buf[len++] = '0' + sc_val % 10;
        buf[len] = '\0';
    }
    DrawTextUTF8(DINO_SCORE_X, DINO_SCORE_Y, DEFAULT_FONT, buf, ink, 0, 0);
}

static void draw_game_over(DinoGame *g)
{
    if (!g->blink) return;

    WORD_COLOR ink = g->night ? WHITE : BLACK;

    char go_text[] = "GAME OVER";
    int tw = GetTextWidthUTF8(DEFAULT_FONT, go_text, 0);
    int th = GetTextHightUTF8(DEFAULT_FONT);
    int tx = (SCREEN_HOR_SIZE - tw) / 2;
    int ty = (SCREEN_VER_SIZE - th) / 2 - 4;

    WORD_COLOR bg = g->night ? BLACK : WHITE;
    SetColor(bg);
    Bar(tx - 2, ty - 1, tx + tw + 2, ty + th + 1);

    SetColor(ink);
    Bevel(tx - 3, ty - 2, tx + tw + 3, ty + th + 2, 0);
    DrawTextUTF8(tx, ty, DEFAULT_FONT, go_text, ink, 0, 0);
}

static void draw_idle_screen(DinoGame *g)
{
    WORD_COLOR ink = BLACK;
    SetColor(ink);

    draw_sprite(54, DINO_GROUND_Y - DINO_RUN1_H, dino_run1, DINO_RUN1_W, DINO_RUN1_H, 3);
    Line(0, DINO_HORIZON_Y, SCREEN_HOR_SIZE - 1, DINO_HORIZON_Y);

    char start_text[] = "PRESS JUMP";
    int tw = GetTextWidthUTF8(DEFAULT_FONT, start_text, 0);
    DrawTextUTF8((SCREEN_HOR_SIZE - tw) / 2, 10, DEFAULT_FONT, start_text, ink, 0, 0);
}

/* ================================================================
 *  Full render (called every GUI_REFR_PERIOD via timer)
 * ================================================================ */

static void dino_render(DinoGame *g)
{
    WORD_COLOR bg = g->night ? BLACK : WHITE;

    SetColor(bg);
    Bar(0, 0, SCREEN_HOR_SIZE - 1, SCREEN_VER_SIZE - 1);

    if (g->state == DINO_STATE_IDLE) {
        draw_idle_screen(g);
    } else {
        WORD_COLOR cloud_color = g->night ? WHITE : BLACK;
        for (int i = 0; i < 2; i++)
            draw_cloud(g->cloud_x[i], g->cloud_y[i], cloud_color);

        draw_ground(g);
        draw_obstacles(g);
        draw_dino(g);
        draw_score(g);

        if (g->state == DINO_STATE_GAME_OVER)
            draw_game_over(g);
    }

    GSP_Display_Refresh();
}

/* ================================================================
 *  Timer callback — called periodically by GUI_GSP timer subsystem
 * ================================================================ */

static void dino_timer_cb(void *param)
{
    (void)param;
    DinoGame *g = &game;

    int key = 0;
    if (GSP_KeyPoll()) {
        key = GSP_ReadKey();
    }

    game_tick(g, key);
    dino_render(g);
}

/* ================================================================
 *  Public API
 * ================================================================ */

void DinoGame_Create(_GSP_Screen *screen)
{
    memset(&game, 0, sizeof(game));
    game.state = DINO_STATE_IDLE;
    game.rng = Timer_GUI ? Timer_GUI : 12345;
    game.hi_score = 0;

    _GSP_Screen *scr = screen;
    if (!scr) {
        scr = Crate_Screen(0, 0, SCREEN_HOR_SIZE, SCREEN_VER_SIZE, 100);
    }
    Set_Active_Screen(scr);

#ifdef USED_TIMER
    CreateTimer(scr, dino_timer_cb, 50, -1);
#endif

    dino_render(&game);
}
