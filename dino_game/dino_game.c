#include "dino_game.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static dino_game_t game;

static lv_color_t bg_day_color;
static lv_color_t bg_night_color;
static lv_color_t fg_day_color;
static lv_color_t fg_night_color;

/* ---------- helpers ---------- */

static int rand_range(int lo, int hi) {
    if (hi <= lo) return lo;
    return lo + rand() % (hi - lo + 1);
}

static lv_color_t current_fg(void) {
    return game.night_mode ? fg_night_color : fg_day_color;
}

static lv_color_t current_bg(void) {
    return game.night_mode ? bg_night_color : bg_day_color;
}

/* ================================================================
 *  Pixel-art sprite data
 * ================================================================ */

#define DINO_SPRITE_W 12
#define DINO_SPRITE_H 16
#define DINO_SCALE    3

static const uint8_t dino_run1[] = {
    0,0,0,0,0,0,1,1,1,1,1,1,
    0,0,0,0,0,1,1,1,1,1,1,1,
    0,0,0,0,0,1,1,0,1,1,1,1,
    0,0,0,0,0,1,1,1,1,1,1,1,
    0,0,0,0,0,1,1,1,1,1,1,1,
    0,0,0,0,0,1,1,1,1,0,0,0,
    0,0,0,0,0,1,1,1,1,1,1,0,
    1,0,0,0,1,1,1,1,1,0,0,0,
    1,0,0,1,1,1,1,1,1,1,0,0,
    1,1,0,1,1,1,1,1,1,0,0,0,
    1,1,1,1,1,1,1,1,1,0,0,0,
    0,1,1,1,1,1,1,1,0,0,0,0,
    0,0,1,1,1,1,1,1,0,0,0,0,
    0,0,0,1,1,1,1,0,0,0,0,0,
    0,0,0,1,1,0,1,1,0,0,0,0,
    0,0,0,1,1,0,0,1,0,0,0,0,
};

static const uint8_t dino_run2[] = {
    0,0,0,0,0,0,1,1,1,1,1,1,
    0,0,0,0,0,1,1,1,1,1,1,1,
    0,0,0,0,0,1,1,0,1,1,1,1,
    0,0,0,0,0,1,1,1,1,1,1,1,
    0,0,0,0,0,1,1,1,1,1,1,1,
    0,0,0,0,0,1,1,1,1,0,0,0,
    0,0,0,0,0,1,1,1,1,1,1,0,
    1,0,0,0,1,1,1,1,1,0,0,0,
    1,0,0,1,1,1,1,1,1,1,0,0,
    1,1,0,1,1,1,1,1,1,0,0,0,
    1,1,1,1,1,1,1,1,1,0,0,0,
    0,1,1,1,1,1,1,1,0,0,0,0,
    0,0,1,1,1,1,1,1,0,0,0,0,
    0,0,0,1,1,1,1,0,0,0,0,0,
    0,0,0,1,0,0,1,1,0,0,0,0,
    0,0,0,0,0,0,1,1,0,0,0,0,
};

#define DINO_DUCK_W 16
#define DINO_DUCK_H 10
#define DINO_DUCK_SCALE 3

static const uint8_t dino_duck1[] = {
    0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,
    0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,
    0,0,0,0,0,0,0,0,0,1,1,0,1,1,1,1,
    0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,
    1,1,0,0,1,1,1,1,1,1,1,1,1,1,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,
    0,0,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
    0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,
    0,0,0,0,0,1,0,0,1,0,0,0,0,0,0,0,
    0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,
};

static const uint8_t dino_duck2[] = {
    0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,
    0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,
    0,0,0,0,0,0,0,0,0,1,1,0,1,1,1,1,
    0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,
    1,1,0,0,1,1,1,1,1,1,1,1,1,1,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,
    0,0,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
    0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,
    0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,
};

static const uint8_t cactus_small_data[] = {
    0,0,0,1,0,0,
    0,0,0,1,0,0,
    0,0,0,1,0,0,
    0,0,0,1,0,0,
    0,1,0,1,0,0,
    0,1,0,1,0,1,
    0,1,0,1,0,1,
    0,1,1,1,0,1,
    0,0,0,1,1,1,
    0,0,0,1,0,0,
    0,0,0,1,0,0,
    0,0,0,1,0,0,
};
#define CACTUS_S_W 6
#define CACTUS_S_H 12
#define CACTUS_S_SCALE 3

static const uint8_t cactus_large_data[] = {
    0,0,0,0,1,1,0,0,
    0,0,0,0,1,1,0,0,
    0,0,0,0,1,1,0,0,
    0,1,0,0,1,1,0,0,
    0,1,0,0,1,1,0,1,
    0,1,0,0,1,1,0,1,
    0,1,1,0,1,1,0,1,
    0,1,1,0,1,1,1,1,
    0,0,0,0,1,1,0,0,
    0,0,0,0,1,1,0,0,
    0,0,0,0,1,1,0,0,
    0,0,0,0,1,1,0,0,
    0,0,0,0,1,1,0,0,
    0,0,0,0,1,1,0,0,
    0,0,0,0,1,1,0,0,
    0,0,0,0,1,1,0,0,
};
#define CACTUS_L_W 8
#define CACTUS_L_H 16
#define CACTUS_L_SCALE 3

static const uint8_t bird_frame1[] = {
    0,0,0,0,1,0,0,0,0,0,
    0,0,0,1,1,0,0,0,0,0,
    0,0,1,1,1,0,0,0,0,0,
    0,0,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,
    0,0,0,0,0,1,1,1,0,0,
    0,0,0,0,0,0,1,0,0,0,
};

static const uint8_t bird_frame2[] = {
    0,0,0,0,0,0,1,0,0,0,
    0,0,0,0,0,1,1,1,0,0,
    1,1,1,1,1,1,1,1,1,1,
    0,0,1,1,1,1,1,1,1,1,
    0,0,1,1,1,0,0,0,0,0,
    0,0,0,1,1,0,0,0,0,0,
    0,0,0,0,1,0,0,0,0,0,
};
#define BIRD_W 10
#define BIRD_H 7
#define BIRD_SCALE 4

static const uint8_t cloud_data[] = {
    0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,
    0,0,0,1,1,1,1,1,0,0,0,1,0,0,0,
    0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};
#define CLOUD_W 15
#define CLOUD_H 5
#define CLOUD_SCALE 3

/* ================================================================
 *  Canvas sprite helpers
 * ================================================================ */

static void draw_sprite(lv_obj_t *canvas, const uint8_t *sprite,
                        int sw, int sh, int scale, lv_color_t fg, lv_color_t bg)
{
    lv_draw_rect_dsc_t rect;
    lv_draw_rect_dsc_init(&rect);
    rect.bg_opa = LV_OPA_COVER;
    rect.border_width = 0;
    rect.radius = 0;

    lv_canvas_fill_bg(canvas, bg, LV_OPA_COVER);

    rect.bg_color = fg;
    for (int y = 0; y < sh; y++) {
        for (int x = 0; x < sw; x++) {
            if (sprite[y * sw + x]) {
                lv_canvas_draw_rect(canvas, x * scale, y * scale, scale, scale, &rect);
            }
        }
    }
}

/* Allocate a canvas with a tracked buffer */
static lv_obj_t *alloc_canvas(lv_obj_t *parent, int pw, int ph, lv_color_t **out_buf) {
    lv_obj_t *c = lv_canvas_create(parent);
    lv_color_t *buf = (lv_color_t *)lv_mem_alloc((uint32_t)(pw * ph) * sizeof(lv_color_t));
    if (out_buf) *out_buf = buf;
    lv_canvas_set_buffer(c, buf, pw, ph, LV_IMG_CF_TRUE_COLOR);
    lv_obj_set_size(c, pw, ph);
    lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
    return c;
}

/* Re-allocate a canvas buffer if the size changed. */
static void realloc_canvas(lv_obj_t *canvas, lv_color_t **buf_ptr, int pw, int ph) {
    if (*buf_ptr) lv_mem_free(*buf_ptr);
    lv_color_t *buf = (lv_color_t *)lv_mem_alloc((uint32_t)(pw * ph) * sizeof(lv_color_t));
    *buf_ptr = buf;
    lv_canvas_set_buffer(canvas, buf, pw, ph, LV_IMG_CF_TRUE_COLOR);
    lv_obj_set_size(canvas, pw, ph);
}

/* ================================================================
 *  DINO
 * ================================================================ */

static void create_dino(void) {
    int pw = DINO_SPRITE_W * DINO_SCALE;
    int ph = DINO_SPRITE_H * DINO_SCALE;
    game.dino_obj = alloc_canvas(game.screen, pw, ph, &game.dino_buf);
    draw_sprite(game.dino_obj, dino_run1, DINO_SPRITE_W, DINO_SPRITE_H,
                DINO_SCALE, current_fg(), current_bg());
    lv_obj_set_pos(game.dino_obj, DINO_X, DINO_GROUND_Y);
}

static void update_dino_sprite(void) {
    const uint8_t *spr;
    int sw, sh, sc;

    if (game.ducking && game.dino_on_ground) {
        sw = DINO_DUCK_W; sh = DINO_DUCK_H; sc = DINO_DUCK_SCALE;
        spr = (game.dino_frame == 0) ? dino_duck1 : dino_duck2;
    } else {
        sw = DINO_SPRITE_W; sh = DINO_SPRITE_H; sc = DINO_SCALE;
        spr = (game.dino_frame == 0) ? dino_run1 : dino_run2;
    }

    int pw = sw * sc;
    int ph = sh * sc;
    realloc_canvas(game.dino_obj, &game.dino_buf, pw, ph);
    draw_sprite(game.dino_obj, spr, sw, sh, sc, current_fg(), current_bg());

    if (game.ducking && game.dino_on_ground) {
        lv_obj_set_pos(game.dino_obj, DINO_X, GROUND_Y - ph);
    }
}

/* ================================================================
 *  GROUND
 * ================================================================ */

static lv_style_t style_ground;

static void create_ground(void) {
    static lv_point_t pts[] = { {0, 0}, {SCREEN_WIDTH, 0} };
    game.ground_line = lv_line_create(game.screen);
    lv_line_set_points(game.ground_line, pts, 2);
    lv_obj_set_pos(game.ground_line, 0, GROUND_Y);

    lv_style_init(&style_ground);
    lv_style_set_line_width(&style_ground, GROUND_HEIGHT);
    lv_style_set_line_color(&style_ground, current_fg());
    lv_obj_add_style(game.ground_line, &style_ground, 0);

    game.ground_dot_count = 0;
    for (int i = 0; i < 40; i++) {
        lv_obj_t *d = lv_obj_create(game.screen);
        lv_obj_set_size(d, rand_range(2, 6), rand_range(1, 3));
        lv_obj_set_style_bg_color(d, current_fg(), 0);
        lv_obj_set_style_bg_opa(d, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(d, 0, 0);
        lv_obj_set_style_radius(d, 0, 0);
        lv_obj_set_style_pad_all(d, 0, 0);
        lv_obj_clear_flag(d, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_pos(d, rand_range(0, SCREEN_WIDTH),
                       GROUND_Y + rand_range(4, 15));
        game.ground_dots[i] = d;
        game.ground_dot_count++;
    }
}

static void update_ground(void) {
    for (int i = 0; i < game.ground_dot_count; i++) {
        lv_coord_t x = lv_obj_get_x(game.ground_dots[i]);
        x -= (int)game.speed;
        if (x < -10) {
            x = SCREEN_WIDTH + rand_range(0, 50);
            lv_obj_set_pos(game.ground_dots[i], x,
                           GROUND_Y + rand_range(4, 15));
            lv_obj_set_size(game.ground_dots[i], rand_range(2, 6), rand_range(1, 3));
        } else {
            lv_obj_set_x(game.ground_dots[i], x);
        }
    }
}

/* ================================================================
 *  OBSTACLES
 * ================================================================ */

static void create_obstacle_obj(obstacle_t *obs) {
    const uint8_t *spr = NULL;
    int sw = 0, sh = 0, sc = 0;

    switch (obs->type) {
        case OBSTACLE_CACTUS_SMALL:
            spr = cactus_small_data; sw = CACTUS_S_W; sh = CACTUS_S_H; sc = CACTUS_S_SCALE;
            break;
        case OBSTACLE_CACTUS_LARGE:
            spr = cactus_large_data; sw = CACTUS_L_W; sh = CACTUS_L_H; sc = CACTUS_L_SCALE;
            break;
        case OBSTACLE_BIRD:
            spr = bird_frame1; sw = BIRD_W; sh = BIRD_H; sc = BIRD_SCALE;
            break;

        case OBSTACLE_CACTUS_GROUP: {
            int pw1 = CACTUS_S_W * CACTUS_S_SCALE;
            int ph1 = CACTUS_S_H * CACTUS_S_SCALE;
            int total_w = pw1 * 3;

            obs->obj = alloc_canvas(game.screen, total_w, ph1, &obs->buf);
            obs->w = total_w;
            obs->h = ph1;
            obs->y = GROUND_Y - ph1;

            lv_draw_rect_dsc_t rect;
            lv_draw_rect_dsc_init(&rect);
            rect.bg_opa = LV_OPA_COVER;
            rect.border_width = 0;
            rect.radius = 0;

            lv_canvas_fill_bg(obs->obj, current_bg(), LV_OPA_COVER);
            rect.bg_color = current_fg();
            for (int c = 0; c < 3; c++) {
                int ox = c * pw1;
                for (int y = 0; y < CACTUS_S_H; y++)
                    for (int x = 0; x < CACTUS_S_W; x++)
                        if (cactus_small_data[y * CACTUS_S_W + x])
                            lv_canvas_draw_rect(obs->obj,
                                ox + x * CACTUS_S_SCALE, y * CACTUS_S_SCALE,
                                CACTUS_S_SCALE, CACTUS_S_SCALE, &rect);
            }

            lv_obj_set_pos(obs->obj, (int)obs->x, obs->y);
            return;
        }
        default:
            return;
    }

    if (!spr) return;

    int pw = sw * sc, ph = sh * sc;
    obs->obj = alloc_canvas(game.screen, pw, ph, &obs->buf);
    obs->w = pw;
    obs->h = ph;
    obs->y = (obs->type == OBSTACLE_BIRD)
        ? ((rand() % 2) ? (GROUND_Y - 30) : (GROUND_Y - 75))
        : (GROUND_Y - ph);

    draw_sprite(obs->obj, spr, sw, sh, sc, current_fg(), current_bg());
    lv_obj_set_pos(obs->obj, (int)obs->x, obs->y);
}

static void destroy_obstacle(obstacle_t *obs) {
    if (obs->obj) {
        lv_obj_del(obs->obj);
        obs->obj = NULL;
    }
    if (obs->buf) {
        lv_mem_free(obs->buf);
        obs->buf = NULL;
    }
    obs->type = OBSTACLE_NONE;
}

static void spawn_obstacle(void) {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (game.obstacles[i].type != OBSTACLE_NONE) continue;

        int r = rand_range(0, 100);
        if (game.score > 200 && r < 20)
            game.obstacles[i].type = OBSTACLE_BIRD;
        else if (r < 40)
            game.obstacles[i].type = OBSTACLE_CACTUS_GROUP;
        else if (r < 70)
            game.obstacles[i].type = OBSTACLE_CACTUS_LARGE;
        else
            game.obstacles[i].type = OBSTACLE_CACTUS_SMALL;

        game.obstacles[i].x = SCREEN_WIDTH + 20;
        game.obstacles[i].bird_frame = 0;
        create_obstacle_obj(&game.obstacles[i]);
        game.next_obstacle_dist = (float)rand_range(200, 400);
        return;
    }
}

static void update_obstacles(void) {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        obstacle_t *o = &game.obstacles[i];
        if (o->type == OBSTACLE_NONE) continue;

        o->x -= game.speed;

        if (o->type == OBSTACLE_BIRD && o->obj) {
            o->bird_frame = (o->bird_frame + 1) % 16;
            const uint8_t *bspr = (o->bird_frame < 8) ? bird_frame1 : bird_frame2;
            draw_sprite(o->obj, bspr, BIRD_W, BIRD_H, BIRD_SCALE,
                        current_fg(), current_bg());
        }

        if (o->x < -(o->w + 20)) {
            destroy_obstacle(o);
        } else {
            lv_obj_set_pos(o->obj, (int)o->x, o->y);
        }
    }

    game.next_obstacle_dist -= game.speed;
    if (game.next_obstacle_dist <= 0)
        spawn_obstacle();
}

/* ================================================================
 *  CLOUDS
 * ================================================================ */

static void create_clouds(void) {
    int pw = CLOUD_W * CLOUD_SCALE, ph = CLOUD_H * CLOUD_SCALE;
    lv_color_t cloud_col = lv_color_make(170, 170, 170);

    for (int i = 0; i < MAX_CLOUDS; i++) {
        game.clouds[i].x = (float)rand_range(0, SCREEN_WIDTH);
        game.clouds[i].y = rand_range(50, 200);
        game.clouds[i].obj = alloc_canvas(game.screen, pw, ph, &game.clouds[i].buf);
        draw_sprite(game.clouds[i].obj, cloud_data, CLOUD_W, CLOUD_H,
                    CLOUD_SCALE, cloud_col, current_bg());
        lv_obj_set_pos(game.clouds[i].obj, (int)game.clouds[i].x, game.clouds[i].y);
    }
}

static void update_clouds(void) {
    lv_color_t cc = game.night_mode
        ? lv_color_make(80, 80, 80)
        : lv_color_make(170, 170, 170);

    for (int i = 0; i < MAX_CLOUDS; i++) {
        game.clouds[i].x -= game.speed * 0.3f;
        if (game.clouds[i].x < -(CLOUD_W * CLOUD_SCALE + 10)) {
            game.clouds[i].x = (float)(SCREEN_WIDTH + rand_range(20, 200));
            game.clouds[i].y = rand_range(50, 200);
        }
        lv_obj_set_pos(game.clouds[i].obj,
                       (int)game.clouds[i].x, game.clouds[i].y);
        draw_sprite(game.clouds[i].obj, cloud_data, CLOUD_W, CLOUD_H,
                    CLOUD_SCALE, cc, current_bg());
    }
}

/* ================================================================
 *  STARS (night mode only)
 * ================================================================ */

#define STAR_SIZE 4

static void create_stars(void) {
    for (int i = 0; i < MAX_STARS; i++) {
        game.stars[i].x = rand_range(0, SCREEN_WIDTH);
        game.stars[i].y = rand_range(20, 250);
        lv_obj_t *s = lv_obj_create(game.screen);
        lv_obj_set_size(s, STAR_SIZE, STAR_SIZE);
        lv_obj_set_style_bg_color(s, fg_night_color, 0);
        lv_obj_set_style_bg_opa(s, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(s, 0, 0);
        lv_obj_set_style_radius(s, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_pad_all(s, 0, 0);
        lv_obj_clear_flag(s, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_pos(s, game.stars[i].x, game.stars[i].y);
        lv_obj_add_flag(s, LV_OBJ_FLAG_HIDDEN);
        game.stars[i].obj = s;
    }
}

static void update_stars(void) {
    for (int i = 0; i < MAX_STARS; i++) {
        if (game.night_mode) {
            lv_obj_clear_flag(game.stars[i].obj, LV_OBJ_FLAG_HIDDEN);
            game.stars[i].x -= (int)(game.speed * 0.1f);
            if (game.stars[i].x < -10) {
                game.stars[i].x = SCREEN_WIDTH + rand_range(0, 50);
                game.stars[i].y = rand_range(20, 250);
            }
            lv_obj_set_pos(game.stars[i].obj, game.stars[i].x, game.stars[i].y);
        } else {
            lv_obj_add_flag(game.stars[i].obj, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/* ================================================================
 *  COLLISION
 * ================================================================ */

static bool check_collision(void) {
    int dx = DINO_X + 6;
    int dy, dw, dh;

    if (game.ducking && game.dino_on_ground) {
        int ph = DINO_DUCK_H * DINO_DUCK_SCALE;
        dy = GROUND_Y - ph + 6;
        dw = DINO_DUCK_W * DINO_DUCK_SCALE - 12;
        dh = ph - 12;
    } else {
        dy = (int)game.dino_y + 6;
        dw = DINO_SPRITE_W * DINO_SCALE - 12;
        dh = DINO_SPRITE_H * DINO_SCALE - 12;
    }

    for (int i = 0; i < MAX_OBSTACLES; i++) {
        obstacle_t *o = &game.obstacles[i];
        if (o->type == OBSTACLE_NONE) continue;

        int ox = (int)o->x + 6;
        int oy = o->y + 6;
        int ow = o->w - 12;
        int oh = o->h - 12;

        if (dx < ox + ow && dx + dw > ox &&
            dy < oy + oh && dy + dh > oy)
            return true;
    }
    return false;
}

/* ================================================================
 *  SCORE
 * ================================================================ */

static void update_score_display(void) {
    char buf[64];
    lv_snprintf(buf, sizeof(buf), "%05d", game.score);
    lv_label_set_text(game.score_label, buf);

    if (game.high_score > 0) {
        lv_snprintf(buf, sizeof(buf), "HI %05d", game.high_score);
        lv_label_set_text(game.hi_score_label, buf);
    }
}

static void create_score(void) {
    game.score_label = lv_label_create(game.screen);
    lv_obj_set_style_text_color(game.score_label, current_fg(), 0);
    lv_obj_set_style_text_font(game.score_label, &lv_font_montserrat_20, 0);
    lv_obj_align(game.score_label, LV_ALIGN_TOP_RIGHT, -20, 15);
    lv_label_set_text(game.score_label, "00000");

    game.hi_score_label = lv_label_create(game.screen);
    lv_obj_set_style_text_color(game.hi_score_label, lv_color_make(115, 115, 115), 0);
    lv_obj_set_style_text_font(game.hi_score_label, &lv_font_montserrat_16, 0);
    lv_obj_align(game.hi_score_label, LV_ALIGN_TOP_RIGHT, -100, 18);
    lv_label_set_text(game.hi_score_label, "");
}

/* ================================================================
 *  GAME OVER / RESTART
 * ================================================================ */

static void show_game_over(void) {
    game.game_over_label = lv_label_create(game.screen);
    lv_obj_set_style_text_color(game.game_over_label, current_fg(), 0);
    lv_obj_set_style_text_font(game.game_over_label, &lv_font_montserrat_24, 0);
    lv_label_set_text(game.game_over_label, "G A M E   O V E R");
    lv_obj_align(game.game_over_label, LV_ALIGN_CENTER, 0, -40);

    game.restart_label = lv_label_create(game.screen);
    lv_obj_set_style_text_color(game.restart_label, lv_color_make(115, 115, 115), 0);
    lv_obj_set_style_text_font(game.restart_label, &lv_font_montserrat_16, 0);
    lv_label_set_text(game.restart_label, "Press SPACE to restart");
    lv_obj_align(game.restart_label, LV_ALIGN_CENTER, 0, 0);
}

static void restart_game(void) {
    if (game.game_over_label) { lv_obj_del(game.game_over_label); game.game_over_label = NULL; }
    if (game.restart_label)   { lv_obj_del(game.restart_label);   game.restart_label = NULL;   }

    for (int i = 0; i < MAX_OBSTACLES; i++)
        destroy_obstacle(&game.obstacles[i]);

    if (game.score > game.high_score)
        game.high_score = game.score;

    game.dino_y        = DINO_GROUND_Y;
    game.dino_vy       = 0;
    game.dino_on_ground = true;
    game.dino_frame    = 0;
    game.ducking       = false;
    game.speed         = INITIAL_SPEED;
    game.distance      = 0;
    game.score         = 0;
    game.game_over     = false;
    game.running       = true;
    game.frame_count   = 0;
    game.next_obstacle_dist = 80;
    game.night_mode    = false;

    lv_obj_set_style_bg_color(game.screen, current_bg(), 0);
    lv_obj_set_pos(game.dino_obj, DINO_X, DINO_GROUND_Y);
    update_dino_sprite();
    update_score_display();
}

/* ================================================================
 *  NIGHT MODE TOGGLE
 * ================================================================ */

static void update_night_mode(void) {
    bool want = ((game.score / 700) % 2) == 1;
    if (want == game.night_mode) return;

    game.night_mode = want;
    lv_obj_set_style_bg_color(game.screen, current_bg(), 0);
    lv_obj_set_style_text_color(game.score_label, current_fg(), 0);
    lv_style_set_line_color(&style_ground, current_fg());
    lv_obj_report_style_change(&style_ground);
    for (int i = 0; i < game.ground_dot_count; i++)
        lv_obj_set_style_bg_color(game.ground_dots[i], current_fg(), 0);
}

/* ================================================================
 *  GAME TICK
 * ================================================================ */

static void game_tick(lv_timer_t *timer) {
    (void)timer;
    if (!game.running || game.game_over) return;

    game.frame_count++;

    /* physics */
    if (!game.dino_on_ground) {
        game.dino_vy += GRAVITY;
        game.dino_y  += game.dino_vy;
        if (game.dino_y >= DINO_GROUND_Y) {
            game.dino_y  = DINO_GROUND_Y;
            game.dino_vy = 0;
            game.dino_on_ground = true;
        }
        lv_obj_set_y(game.dino_obj, (int)game.dino_y);
    }

    /* dino animation */
    if (game.frame_count % 6 == 0) {
        game.dino_frame = (game.dino_frame + 1) % 2;
        update_dino_sprite();
    }

    update_obstacles();
    update_ground();
    update_clouds();
    update_stars();

    /* score */
    game.distance += game.speed;
    game.score = (int)(game.distance / 10);
    update_score_display();

    /* speed ramp */
    game.speed += SPEED_INCREMENT;
    if (game.speed > MAX_SPEED) game.speed = MAX_SPEED;

    update_night_mode();

    /* collision */
    if (check_collision()) {
        game.game_over = true;
        game.running   = false;
        show_game_over();
    }
}

/* ================================================================
 *  KEYBOARD INPUT
 * ================================================================ */

static void key_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(e);

        if (game.game_over) {
            if (key == ' ' || key == LV_KEY_UP || key == LV_KEY_ENTER) restart_game();
            return;
        }

        if (!game.running) {
            if (key == ' ' || key == LV_KEY_UP || key == LV_KEY_ENTER) {
                game.running = true;
                game.dino_vy = JUMP_VELOCITY;
                game.dino_on_ground = false;
                if (game.start_label) {
                    lv_obj_del(game.start_label);
                    game.start_label = NULL;
                }
            }
            return;
        }

        if (key == ' ' || key == LV_KEY_UP) {
            if (game.dino_on_ground) {
                game.dino_vy = JUMP_VELOCITY;
                game.dino_on_ground = false;
                game.ducking = false;
            }
        }

        if (key == LV_KEY_DOWN) {
            game.ducking = true;
            if (game.dino_on_ground)
                update_dino_sprite();
            else
                game.dino_vy += 3;
        }
    }

    if (code == LV_EVENT_RELEASED) {
        game.ducking = false;
        if (game.dino_on_ground)
            update_dino_sprite();
    }
}

/* ================================================================
 *  PUBLIC ENTRY POINT
 * ================================================================ */

void dino_game_create(void) {
    srand((unsigned)time(NULL));

    bg_day_color   = lv_color_make(247, 247, 247);
    bg_night_color = lv_color_make(32,  33,  36);
    fg_day_color   = lv_color_make(83,  83,  83);
    fg_night_color = lv_color_make(172, 172, 172);

    memset(&game, 0, sizeof(game));
    game.dino_y         = DINO_GROUND_Y;
    game.dino_on_ground = true;
    game.speed          = INITIAL_SPEED;
    game.next_obstacle_dist = 150;

    /* main container */
    game.screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(game.screen, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_center(game.screen);
    lv_obj_set_style_bg_color(game.screen, bg_day_color, 0);
    lv_obj_set_style_bg_opa(game.screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(game.screen, 0, 0);
    lv_obj_set_style_radius(game.screen, 0, 0);
    lv_obj_set_style_pad_all(game.screen, 0, 0);
    lv_obj_clear_flag(game.screen, LV_OBJ_FLAG_SCROLLABLE);

    create_ground();
    create_clouds();
    create_stars();
    create_dino();
    create_score();

    /* start hint */
    game.start_label = lv_label_create(game.screen);
    lv_obj_set_style_text_color(game.start_label, lv_color_make(115, 115, 115), 0);
    lv_obj_set_style_text_font(game.start_label, &lv_font_montserrat_16, 0);
    lv_label_set_text(game.start_label, "Press SPACE or UP to start");
    lv_obj_align(game.start_label, LV_ALIGN_CENTER, 0, -20);

    /* keyboard group */
    lv_group_t *grp = lv_group_create();
    lv_group_add_obj(grp, game.screen);
    lv_obj_add_flag(game.screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(game.screen, key_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(game.screen, key_cb, LV_EVENT_RELEASED, NULL);

    lv_indev_t *indev = lv_indev_get_next(NULL);
    while (indev) {
        if (lv_indev_get_type(indev) == LV_INDEV_TYPE_KEYPAD ||
            lv_indev_get_type(indev) == LV_INDEV_TYPE_ENCODER)
            lv_indev_set_group(indev, grp);
        indev = lv_indev_get_next(indev);
    }
    lv_group_focus_obj(game.screen);

    game.game_timer = lv_timer_create(game_tick, GAME_TICK_MS, NULL);
}
