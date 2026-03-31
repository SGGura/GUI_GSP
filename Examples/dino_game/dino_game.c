/**
 * @file dino_game.c
 * @brief Chrome-style T-Rex dinosaur runner — portable game logic for 128x64.
 */
#include "dino_game.h"
#include <string.h>
#include <stdlib.h>

/* ======================== Tuning constants ======================== */
#define GRAVITY         0.55f
#define JUMP_VEL       -6.5f
#define INITIAL_SPEED   2.0f
#define MAX_SPEED       5.5f
#define SPEED_INC       0.0008f
#define SCORE_PER_TICK  1

#define MIN_SPAWN_GAP   40
#define MAX_SPAWN_GAP   80
#define BIRD_FLAP_RATE  8

/* ======================== Simple PRNG ======================== */
static uint32_t rng_state = 12345;

static uint32_t rng_next(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static int rng_range(int lo, int hi)
{
    if (lo >= hi) return lo;
    return lo + (int)(rng_next() % (uint32_t)(hi - lo + 1));
}

/* ======================== Sprite data (1-bit bitmaps) ======================== */

/*
 * Sprites are stored as arrays of bytes, row-major, 1 bit per pixel.
 * Bit 7 = leftmost pixel.
 * A '1' bit means foreground (black on white / white on night).
 */

/* Dino standing / running frame 1: 11 wide x 12 tall */
#define DINO_W  11
#define DINO_H  12
static const uint8_t sprite_dino_run1[] = {
    0x07, 0xC0,  /*  .....XXXXX..... */
    0x07, 0xE0,  /*  .....XXXXXX.... */
    0x07, 0xE0,  /*  .....XXXXXX.... */
    0x07, 0x00,  /*  .....XXX....... */
    0x1F, 0xC0,  /*  ...XXXXXXX..... */
    0x3F, 0x80,  /*  ..XXXXXXX...... */
    0xFF, 0x80,  /*  XXXXXXXXX...... */
    0xF7, 0x80,  /*  XXXX.XXXX...... */
    0x7F, 0x00,  /*  .XXXXXXX....... */
    0x3E, 0x00,  /*  ..XXXXX........ */
    0x1A, 0x00,  /*  ...XX.X........ */
    0x12, 0x00,  /*  ...X..X........ */
};

/* Dino running frame 2 (alternate legs) */
static const uint8_t sprite_dino_run2[] = {
    0x07, 0xC0,
    0x07, 0xE0,
    0x07, 0xE0,
    0x07, 0x00,
    0x1F, 0xC0,
    0x3F, 0x80,
    0xFF, 0x80,
    0xF7, 0x80,
    0x7F, 0x00,
    0x3E, 0x00,
    0x24, 0x00,
    0x24, 0x00,
};

/* Dino ducking frame 1: 16 wide x 8 tall */
#define DINO_DUCK_W 16
#define DINO_DUCK_H 8
static const uint8_t sprite_dino_duck1[] = {
    0x00, 0x7C,  /* row0 */
    0x00, 0x7E,  /* row1 */
    0x00, 0x7E,  /* row2 */
    0xFF, 0xFC,  /* row3 */
    0xFF, 0xF0,  /* row4 */
    0xFF, 0xF0,  /* row5 */
    0x0D, 0x00,  /* row6 */
    0x09, 0x00,  /* row7 */
};

static const uint8_t sprite_dino_duck2[] = {
    0x00, 0x7C,
    0x00, 0x7E,
    0x00, 0x7E,
    0xFF, 0xFC,
    0xFF, 0xF0,
    0xFF, 0xF0,
    0x12, 0x00,
    0x12, 0x00,
};

/* Small cactus: 5w x 9h */
#define CACT_S_W 5
#define CACT_S_H 9
static const uint8_t sprite_cactus_small[] = {
    0x20,  /* ..X.. */
    0x20,  /* ..X.. */
    0xA8,  /* X.X.X */
    0xA8,  /* X.X.X */
    0xF8,  /* XXXXX */
    0x70,  /* .XXX. */
    0x20,  /* ..X.. */
    0x20,  /* ..X.. */
    0x20,  /* ..X.. */
};

/* Tall cactus: 5w x 14h */
#define CACT_T_W 5
#define CACT_T_H 14
static const uint8_t sprite_cactus_tall[] = {
    0x20,
    0x20,
    0x20,
    0xA8,
    0xA8,
    0xA8,
    0xF8,
    0x70,
    0x20,
    0x20,
    0x20,
    0x20,
    0x20,
    0x20,
};

/* Group of cacti: 11w x 9h */
#define CACT_G_W 11
#define CACT_G_H 9
static const uint8_t sprite_cactus_group[] = {
    0x22, 0x00,  /* ..X...X..... */
    0x22, 0x80,  /* ..X...X.X... */
    0xAA, 0x80,  /* X.X.X.X.X.. */
    0xAA, 0x80,  /* X.X.X.X.X.. */
    0xFB, 0x80,  /* XXXXX.XXX.. */
    0x73, 0x00,  /* .XXX..XX... */
    0x22, 0x00,  /* ..X...X.... */
    0x22, 0x00,  /* ..X...X.... */
    0x22, 0x00,  /* ..X...X.... */
};

/* Bird: 9w x 7h, two frames */
#define BIRD_W 9
#define BIRD_H 7
static const uint8_t sprite_bird_up[] = {
    0x10, 0x00,  /* ...X..... */
    0x38, 0x00,  /* ..XXX.... */
    0x7F, 0x00,  /* .XXXXXXX. */
    0xFF, 0x80,  /* XXXXXXXXX */
    0x06, 0x00,  /* .....XX.. */
    0x00, 0x00,
    0x00, 0x00,
};

static const uint8_t sprite_bird_down[] = {
    0x00, 0x00,
    0x00, 0x00,
    0x7F, 0x00,
    0xFF, 0x80,
    0x06, 0x00,
    0x38, 0x00,
    0x10, 0x00,
};

/* ======================== Sprite drawing ======================== */

static void draw_fill_rect(const DinoDraw *d, int x, int y, int w, int h, int c)
{
    if (d->fill_rect) {
        d->fill_rect(x, y, w, h, c);
        return;
    }
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            d->set_pixel(x + i, y + j, c);
}

static void draw_sprite(const DinoDraw *d, int sx, int sy,
                         const uint8_t *data, int w, int h, int fg)
{
    int bytes_per_row = (w + 7) / 8;
    for (int row = 0; row < h; row++) {
        for (int col = 0; col < w; col++) {
            int byte_idx = row * bytes_per_row + (col / 8);
            int bit = 7 - (col % 8);
            if (data[byte_idx] & (1 << bit)) {
                int px = sx + col;
                int py = sy + row;
                if (px >= 0 && px < DINO_SCR_W && py >= 0 && py < DINO_SCR_H)
                    d->set_pixel(px, py, fg);
            }
        }
    }
}

/* ======================== 3x5 digit font ======================== */

static const uint8_t digits_3x5[10][5] = {
    {0xE0, 0xA0, 0xA0, 0xA0, 0xE0}, /* 0 */
    {0x40, 0xC0, 0x40, 0x40, 0xE0}, /* 1 */
    {0xE0, 0x20, 0xE0, 0x80, 0xE0}, /* 2 */
    {0xE0, 0x20, 0xE0, 0x20, 0xE0}, /* 3 */
    {0xA0, 0xA0, 0xE0, 0x20, 0x20}, /* 4 */
    {0xE0, 0x80, 0xE0, 0x20, 0xE0}, /* 5 */
    {0xE0, 0x80, 0xE0, 0xA0, 0xE0}, /* 6 */
    {0xE0, 0x20, 0x40, 0x40, 0x40}, /* 7 */
    {0xE0, 0xA0, 0xE0, 0xA0, 0xE0}, /* 8 */
    {0xE0, 0xA0, 0xE0, 0x20, 0xE0}, /* 9 */
};

static void draw_digit(const DinoDraw *d, int x, int y, int digit, int fg)
{
    if (digit < 0 || digit > 9) return;
    const uint8_t *glyph = digits_3x5[digit];
    for (int row = 0; row < 5; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 3; col++) {
            if (bits & (0x80 >> col))
                d->set_pixel(x + col, y + row, fg);
        }
    }
}

static void draw_number(const DinoDraw *d, int x, int y, uint32_t num, int ndigits, int fg)
{
    char buf[12];
    int len = 0;
    if (num == 0) {
        buf[len++] = 0;
    } else {
        uint32_t tmp = num;
        while (tmp > 0) {
            buf[len++] = (char)(tmp % 10);
            tmp /= 10;
        }
    }
    while (len < ndigits) buf[len++] = 0;

    for (int i = len - 1; i >= 0; i--) {
        draw_digit(d, x, y, buf[i], fg);
        x += 4;
    }
}

/* ======================== Collision detection ======================== */

static bool rects_overlap(int ax, int ay, int aw, int ah,
                          int bx, int by, int bw, int bh)
{
    if (ax + aw <= bx || bx + bw <= ax) return false;
    if (ay + ah <= by || by + bh <= ay) return false;
    return true;
}

/* ======================== Init ======================== */

void dino_init(DinoGame *g)
{
    memset(g, 0, sizeof(*g));
    g->state = DINO_STATE_IDLE;
    g->dino_x = 10;
    g->dino_y = (float)(DINO_GROUND_Y - DINO_H);
    g->speed = INITIAL_SPEED;

    for (int i = 0; i < DINO_MAX_OBS; i++)
        g->obs[i].kind = OBS_NONE;

    g->next_spawn_dist = 60;

    for (int i = 0; i < DINO_MAX_CLOUDS; i++) {
        g->clouds[i].x = (int16_t)rng_range(0, DINO_SCR_W);
        g->clouds[i].y = (int16_t)rng_range(5, 25);
    }
}

/* ======================== Spawn obstacles ======================== */

static void spawn_obstacle(DinoGame *g)
{
    for (int i = 0; i < DINO_MAX_OBS; i++) {
        if (g->obs[i].kind != OBS_NONE) continue;

        int r = rng_range(0, 100);
        Obstacle *o = &g->obs[i];
        o->bird_frame = 0;

        if (r < 35) {
            o->kind = OBS_CACTUS_SMALL;
            o->w = CACT_S_W;
            o->h = CACT_S_H;
            o->y = DINO_GROUND_Y - CACT_S_H;
        } else if (r < 60) {
            o->kind = OBS_CACTUS_TALL;
            o->w = CACT_T_W;
            o->h = CACT_T_H;
            o->y = DINO_GROUND_Y - CACT_T_H;
        } else if (r < 85) {
            o->kind = OBS_CACTUS_GROUP;
            o->w = CACT_G_W;
            o->h = CACT_G_H;
            o->y = DINO_GROUND_Y - CACT_G_H;
        } else {
            o->kind = OBS_BIRD;
            o->w = BIRD_W;
            o->h = BIRD_H;
            int alt = rng_range(0, 2);
            if (alt == 0)      o->y = DINO_GROUND_Y - 7;
            else if (alt == 1) o->y = DINO_GROUND_Y - 16;
            else               o->y = DINO_GROUND_Y - 26;
        }
        o->x = DINO_SCR_W + 2;

        g->next_spawn_dist = (int16_t)rng_range(MIN_SPAWN_GAP, MAX_SPAWN_GAP);
        return;
    }
}

/* ======================== Tick ======================== */

void dino_tick(DinoGame *g, int dt_ms)
{
    (void)dt_ms;

    if (g->game_over) return;
    if (!g->started) return;

    g->frame_count++;

    /* Speed up over time */
    if (g->speed < MAX_SPEED)
        g->speed += SPEED_INC;

    /* Night mode toggles every 700 frames */
    if (g->frame_count % 700 == 0)
        g->night = !g->night;

    /* Animate dino legs */
    if (g->frame_count % 4 == 0)
        g->dino_frame ^= 1;

    /* Physics: jumping */
    if (g->state == DINO_STATE_JUMPING) {
        g->dino_vy += GRAVITY;
        g->dino_y += g->dino_vy;
        float ground = (float)(DINO_GROUND_Y - DINO_H);
        if (g->dino_y >= ground) {
            g->dino_y = ground;
            g->dino_vy = 0;
            g->state = DINO_STATE_RUNNING;
        }
    }

    /* Move obstacles */
    bool any_active = false;
    int rightmost_x = -1000;
    for (int i = 0; i < DINO_MAX_OBS; i++) {
        if (g->obs[i].kind == OBS_NONE) continue;
        g->obs[i].x -= (int16_t)g->speed;

        if (g->obs[i].kind == OBS_BIRD) {
            if (g->frame_count % BIRD_FLAP_RATE == 0)
                g->obs[i].bird_frame ^= 1;
        }

        if (g->obs[i].x < -20) {
            g->obs[i].kind = OBS_NONE;
        } else {
            any_active = true;
            if (g->obs[i].x > rightmost_x)
                rightmost_x = g->obs[i].x;
        }
    }

    /* Spawn new obstacles */
    if (!any_active || rightmost_x < (DINO_SCR_W - g->next_spawn_dist)) {
        spawn_obstacle(g);
    }

    /* Move clouds */
    for (int i = 0; i < DINO_MAX_CLOUDS; i++) {
        g->clouds[i].x -= 1;
        if (g->clouds[i].x < -12) {
            g->clouds[i].x = DINO_SCR_W + rng_range(0, 30);
            g->clouds[i].y = (int16_t)rng_range(5, 25);
        }
    }

    /* Ground scroll */
    g->ground_offset += g->speed;
    if (g->ground_offset >= 12.0f)
        g->ground_offset -= 12.0f;

    /* Score */
    g->score += SCORE_PER_TICK;

    /* Collision */
    int dw, dh, dy;
    if (g->dino_ducking && g->state != DINO_STATE_JUMPING) {
        dw = DINO_DUCK_W;
        dh = DINO_DUCK_H;
        dy = DINO_GROUND_Y - DINO_DUCK_H;
    } else {
        dw = DINO_W;
        dh = DINO_H;
        dy = (int)g->dino_y;
    }

    /* Shrink hitbox by 2px on each side for fairness */
    int hx = g->dino_x + 2;
    int hy = dy + 2;
    int hw = dw - 4;
    int hh = dh - 4;

    for (int i = 0; i < DINO_MAX_OBS; i++) {
        if (g->obs[i].kind == OBS_NONE) continue;
        if (rects_overlap(hx, hy, hw, hh,
                          g->obs[i].x + 1, g->obs[i].y + 1,
                          g->obs[i].w - 2, g->obs[i].h - 2)) {
            g->state = DINO_STATE_DEAD;
            g->game_over = true;
            if (g->score > g->hi_score)
                g->hi_score = g->score;
            return;
        }
    }
}

/* ======================== Input ======================== */

void dino_jump(DinoGame *g)
{
    if (g->game_over) {
        uint32_t hi = g->hi_score;
        dino_init(g);
        g->hi_score = hi;
        g->started = true;
        g->state = DINO_STATE_RUNNING;
        return;
    }
    if (!g->started) {
        g->started = true;
        g->state = DINO_STATE_RUNNING;
    }
    if (g->state == DINO_STATE_RUNNING) {
        g->state = DINO_STATE_JUMPING;
        g->dino_vy = JUMP_VEL;
    }
}

void dino_duck_start(DinoGame *g)
{
    if (g->game_over || !g->started) return;
    g->dino_ducking = true;
    if (g->state == DINO_STATE_JUMPING) {
        g->dino_vy += 2.0f;
    }
}

void dino_duck_end(DinoGame *g)
{
    g->dino_ducking = false;
}

/* ======================== Draw ======================== */

static void draw_ground(const DinoGame *g, const DinoDraw *d, int fg, int bg)
{
    (void)bg;
    int y = DINO_GROUND_Y;
    int off = (int)g->ground_offset;

    for (int x = 0; x < DINO_SCR_W; x++)
        d->set_pixel(x, y, fg);

    /* Dashes below ground line for texture */
    for (int x = -off; x < DINO_SCR_W; x += 6) {
        int x0 = x;
        if (x0 >= 0 && x0 + 2 < DINO_SCR_W) {
            d->set_pixel(x0, y + 2, fg);
            d->set_pixel(x0 + 1, y + 2, fg);
        }
    }
    for (int x = -off + 3; x < DINO_SCR_W; x += 8) {
        int x0 = x;
        if (x0 >= 0 && x0 + 1 < DINO_SCR_W) {
            d->set_pixel(x0, y + 4, fg);
        }
    }
}

static void draw_cloud(const DinoDraw *d, int cx, int cy, int fg)
{
    /* Tiny 8x3 cloud */
    static const uint8_t cloud_data[] = {
        0x3C,  /* ..XXXX.. */
        0x7E,  /* .XXXXXX. */
        0x3C,  /* ..XXXX.. */
    };
    draw_sprite(d, cx, cy, cloud_data, 8, 3, fg);
}

void dino_draw(const DinoGame *g, const DinoDraw *draw)
{
    int fg = g->night ? 0 : 1;   /* foreground: black(1) normally, white(0) at night */
    int bg = g->night ? 1 : 0;

    /* Clear screen */
    draw_fill_rect(draw, 0, 0, DINO_SCR_W, DINO_SCR_H, bg);

    /* Clouds */
    for (int i = 0; i < DINO_MAX_CLOUDS; i++)
        draw_cloud(draw, g->clouds[i].x, g->clouds[i].y, fg);

    /* Ground */
    draw_ground(g, draw, fg, bg);

    /* Obstacles */
    for (int i = 0; i < DINO_MAX_OBS; i++) {
        const Obstacle *o = &g->obs[i];
        if (o->kind == OBS_NONE) continue;

        switch (o->kind) {
        case OBS_CACTUS_SMALL:
            draw_sprite(draw, o->x, o->y, sprite_cactus_small, CACT_S_W, CACT_S_H, fg);
            break;
        case OBS_CACTUS_TALL:
            draw_sprite(draw, o->x, o->y, sprite_cactus_tall, CACT_T_W, CACT_T_H, fg);
            break;
        case OBS_CACTUS_GROUP:
            draw_sprite(draw, o->x, o->y, sprite_cactus_group, CACT_G_W, CACT_G_H, fg);
            break;
        case OBS_BIRD:
            if (o->bird_frame)
                draw_sprite(draw, o->x, o->y, sprite_bird_down, BIRD_W, BIRD_H, fg);
            else
                draw_sprite(draw, o->x, o->y, sprite_bird_up, BIRD_W, BIRD_H, fg);
            break;
        default:
            break;
        }
    }

    /* Dino */
    if (g->dino_ducking && g->state != DINO_STATE_JUMPING) {
        int dy = DINO_GROUND_Y - DINO_DUCK_H;
        const uint8_t *spr = g->dino_frame ? sprite_dino_duck2 : sprite_dino_duck1;
        draw_sprite(draw, g->dino_x, dy, spr, DINO_DUCK_W, DINO_DUCK_H, fg);
    } else {
        const uint8_t *spr;
        if (g->state == DINO_STATE_IDLE || g->state == DINO_STATE_DEAD)
            spr = sprite_dino_run1;
        else
            spr = g->dino_frame ? sprite_dino_run2 : sprite_dino_run1;
        draw_sprite(draw, g->dino_x, (int)g->dino_y, spr, DINO_W, DINO_H, fg);
    }

    /* Score (top right) */
    draw_number(draw, DINO_SCR_W - 22, 2, g->score / 10, 5, fg);

    /* HI score */
    if (g->hi_score > 0) {
        /* "HI" letters */
        /* H */
        int hx = DINO_SCR_W - 50;
        draw->set_pixel(hx, 2, fg); draw->set_pixel(hx, 3, fg);
        draw->set_pixel(hx, 4, fg); draw->set_pixel(hx, 5, fg);
        draw->set_pixel(hx, 6, fg);
        draw->set_pixel(hx+1, 4, fg);
        draw->set_pixel(hx+2, 2, fg); draw->set_pixel(hx+2, 3, fg);
        draw->set_pixel(hx+2, 4, fg); draw->set_pixel(hx+2, 5, fg);
        draw->set_pixel(hx+2, 6, fg);
        /* I */
        draw->set_pixel(hx+4, 2, fg); draw->set_pixel(hx+4, 3, fg);
        draw->set_pixel(hx+4, 4, fg); draw->set_pixel(hx+4, 5, fg);
        draw->set_pixel(hx+4, 6, fg);

        draw_number(draw, hx + 7, 2, g->hi_score / 10, 5, fg);
    }

    /* Game over text */
    if (g->game_over) {
        /* "GAME OVER" centered — draw as filled rect + inverted text approximation */
        int bx = (DINO_SCR_W - 50) / 2;
        int by = DINO_SCR_H / 2 - 8;

        /* Simple "GAME OVER" pixel art, 49x5 */
        static const char *go_text[] = {
            " XX  XX X X X X XXX  XX X X XXX XX ",
            "X   X X XX XX X     X X X X X   X X",
            "X X XXX X X X XXX   X X X X XX  XX ",
            "X X X X X   X X     X X X X X   X X",
            " XX X X X   X XXX   XX   X  XXX X X",
        };
        for (int r = 0; r < 5; r++)
            for (int c = 0; go_text[r][c]; c++)
                if (go_text[r][c] == 'X')
                    draw->set_pixel(bx + c, by + r, fg);
    }

    /* "Press SPACE" when idle */
    if (!g->started && !g->game_over) {
        /* Blinking effect: show only on even half-seconds */
        if ((g->frame_count / 15) % 2 == 0) {
            /* Tiny arrow up icon above dino */
            int ax = g->dino_x + DINO_W / 2;
            int ay = (int)g->dino_y - 6;
            draw->set_pixel(ax, ay, fg);
            draw->set_pixel(ax - 1, ay + 1, fg);
            draw->set_pixel(ax + 1, ay + 1, fg);
            draw->set_pixel(ax - 2, ay + 2, fg);
            draw->set_pixel(ax + 2, ay + 2, fg);
        }
    }
}
