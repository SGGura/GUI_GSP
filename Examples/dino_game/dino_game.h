/**
 * @file dino_game.h
 * @brief Chrome-style T-Rex dinosaur runner game for GUI_GSP (128x64 monochrome)
 *
 * Controls:
 *   KEY_UP / KEY_OK  — jump  (hold for higher jump)
 *   KEY_DOWN          — duck
 *   KEY_OK            — restart after game over
 */
#ifndef DINO_GAME_H
#define DINO_GAME_H

#include "GUI_GSP/GUI_GSP.h"

/* ==================== Key codes (override in project if different) ==================== */
#ifndef DINO_KEY_JUMP
#define DINO_KEY_JUMP   1
#endif
#ifndef DINO_KEY_DUCK
#define DINO_KEY_DUCK   2
#endif
#ifndef DINO_KEY_OK
#define DINO_KEY_OK     3
#endif

/* ==================== Game tuning ==================== */
#define DINO_GROUND_Y           53
#define DINO_HORIZON_Y          (DINO_GROUND_Y + 1)

#define DINO_GRAVITY            2
#define DINO_JUMP_VELOCITY      (-10)
#define DINO_MAX_FALL            8

#define DINO_START_SPEED        2
#define DINO_MAX_SPEED          5
#define DINO_SPEED_INC_SCORE    100

#define DINO_MAX_OBSTACLES      3
#define DINO_MIN_OBS_GAP        35
#define DINO_MAX_OBS_GAP        70

#define DINO_SCORE_X            90
#define DINO_SCORE_Y            2

/* ==================== Obstacle types ==================== */
typedef enum {
    OBS_NONE = 0,
    OBS_CACTUS_SMALL,
    OBS_CACTUS_LARGE,
    OBS_CACTUS_GROUP,
    OBS_BIRD
} DinoObsType;

/* ==================== Obstacle ==================== */
typedef struct {
    DinoObsType type;
    short       x;
    short       y;
    short       w;
    short       h;
    short       frame;
} DinoObstacle;

/* ==================== Game states ==================== */
typedef enum {
    DINO_STATE_IDLE,
    DINO_STATE_RUNNING,
    DINO_STATE_GAME_OVER
} DinoState;

/* ==================== Main game context ==================== */
typedef struct {
    DinoState       state;
    /* dino position/physics */
    short           dino_x;
    short           dino_y;
    short           dino_vy;
    bool            dino_ducking;
    short           dino_w;
    short           dino_h;
    /* animation */
    unsigned char   anim_frame;
    unsigned int    anim_timer;
    /* obstacles */
    DinoObstacle    obs[DINO_MAX_OBSTACLES];
    short           next_obs_dist;
    /* ground scroll offset */
    short           ground_offset;
    /* clouds */
    short           cloud_x[2];
    short           cloud_y[2];
    /* score */
    unsigned int    score;
    unsigned int    hi_score;
    unsigned char   speed;
    /* night mode */
    bool            night;
    unsigned int    night_timer;
    /* blink game-over text */
    bool            blink;
    unsigned int    blink_timer;
    /* random seed */
    unsigned int    rng;
} DinoGame;

/* ==================== Public API ==================== */

/**
 * Create the Dino game screen and start the game.
 * Call once; then let GUI_Run() drive the main loop.
 * @param screen  Existing GSP screen to attach widgets to (or NULL to auto-create).
 */
void DinoGame_Create(_GSP_Screen *screen);

/**
 * Reset and start a new game round.
 */
void DinoGame_Reset(DinoGame *g);

#endif /* DINO_GAME_H */
