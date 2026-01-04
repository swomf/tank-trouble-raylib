#include "types.h"

#ifndef TANKTROUBLE_CONFIG_H
#define TANKTROUBLE_CONFIG_H

// NOTE: some of the following consts are wrapped in enums because
//       they are compile time constants.

// map generation
static const int SCREEN_W = 2200;        // pixels
static const int SCREEN_H = 1550;        // pixels
enum { MAZE_COLS = 12, MAZE_ROWS = 12 }; // cells
static const int CELL = 120;             // pixels
static const int WALL_T = 8;             // pixels

// Normally, pressing R resets the map and players at any point.
//    TRUE: R only resets when 1 tank remains (win) or 0 tanks remain (draw)
//    FALSE: R resets at any time regardless of remaining tank count
// FALSE is recommended unless someone keeps
// accidentally hitting R before the round ends.
static const bool SAFE_R = false;

// (advanced) map generation: chance to remove extra wall on mazegen
static const float LOOP_CHANCE = 0.2f;

static const int ORIGIN_X = ((SCREEN_W - (MAZE_COLS * CELL)) / 2);
static const int ORIGIN_Y = ((SCREEN_H - (MAZE_ROWS * CELL)) / 2);

// bullet stats
enum { MAX_BULLETS = 10 }; // max bullets PER tank
static const int MAX_BOUNCES = 8;
static const float BULLET_R = 6.0f; // radius
static const float BULLET_SPEED = 840.0f;
static const float BULLET_LIFETIME = 8.0f;

// tank handling and stats
static const float TANK_W = 60.0f;
static const float TANK_H = 44.0f;
static const float TANK_SPEED = 300.0f;
static const float TURN_SPEED = 210.0f;

// cosmetic message only (for top of screen)
static const char *TANK_CONTROLS_MSG = "P1 WASD + C ... "
                                       "P2 IJKL + M ... "
                                       "P3 Arrows + Space ... "
                                       "P4 Numpad 8456 + 0 ... "
                                       "R: new maze";

// clang-format off
// NOTE: after changing tank controls
//       you may want to change the cosmetic message above
static Tank tanks[] = {
  { // TANK 0 (top left)
    .up    = KEY_W,
    .left  = KEY_A,
    .down  = KEY_S, 
    .right = KEY_D,
    .fire  = KEY_C,

    .color      = {0, 228, 48, 255}, // green rgba
    .startHealth = 3,

    .spawnpoint = {
      ORIGIN_X + CELL * 0.5f,  // x
      ORIGIN_Y + CELL * 0.5f}, // y
    .startAngleDeg = 45.0f,
  },
  { // TANK 1 (top right)
    .up    = KEY_I,
    .left  = KEY_J,
    .down  = KEY_K,
    .right = KEY_L,
    .fire  = KEY_M,

    .color = {230, 41, 55, 255}, // red rgba
    .startHealth = 3,

    .spawnpoint = {
      ORIGIN_X + (MAZE_COLS - 1) * CELL + CELL * 0.5f,
      ORIGIN_Y + CELL * 0.5f},
    .startAngleDeg = 135.0f,
  },
  { // TANK 2 (bottom left)
    .up    = KEY_UP,
    .left  = KEY_LEFT,
    .down  = KEY_DOWN,
    .right = KEY_RIGHT,
    .fire  = KEY_SPACE,

    .color = {0, 121, 241, 255}, // blue rgba
    .startHealth = 3,

    .spawnpoint = {
      ORIGIN_X + CELL * 0.5f,
      ORIGIN_Y + (MAZE_ROWS - 1) * CELL + CELL * 0.5f},
    .startAngleDeg = -45.0f,
  },
  { // TANK 3 (bottom right)
    .up = KEY_KP_8,
    .left = KEY_KP_4,
    .down = KEY_KP_5,
    .right = KEY_KP_6,
    .fire = KEY_KP_0,

    .color = {255, 161, 0, 255}, // orange rgba
    .startHealth = 3,

    .spawnpoint = {
      ORIGIN_X + (MAZE_COLS - 1) * CELL + CELL * 0.5f,
      ORIGIN_Y + (MAZE_ROWS - 1) * CELL + CELL * 0.5f},
      .startAngleDeg = -135.0f,
  }
};
// clang-format on

// sound settings
static const float MASTER_VOLUME = 1.0f;

// (advanced) checks to kills a bullet if it spawns inside a wall
// 0: allows wallshots if you go up to a wall
// 1: allows limited wallshots
// 2: recommended
// 3+: not really needed
static const int WALL_SHOT_SAFETY_ITERS = 2;

// derived consts, dont change
enum {
  TOTAL_TANKS = sizeof(tanks) / sizeof(Tank),
  MAX_MAP_BULLETS = MAX_BULLETS * TOTAL_TANKS,
  MAX_WALLS = MAZE_COLS * MAZE_ROWS * 4
};
#endif
