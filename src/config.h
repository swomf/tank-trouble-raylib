#include <raylib.h>

#ifndef TANKTROUBLE_CONFIG_H
#define TANKTROUBLE_CONFIG_H

// map generation
#define SCREEN_W 2200
#define SCREEN_H 1550
#define MAZE_COLS 12
#define MAZE_ROWS 12
#define CELL 120
#define WALL_T 8.0f

// Pressing R resets the map at any point.
//    Remove this comment to prevent R from firing a
//    reset when more than 1 tank remains (i.e. round not ended yet).
#define SAFE_R

// (advanced) map generation: chance to remove extra wall on mazegen
#define LOOP_CHANCE 0.2f

#define ORIGIN_X ((SCREEN_W - (MAZE_COLS * CELL)) / 2)
#define ORIGIN_Y ((SCREEN_H - (MAZE_ROWS * CELL)) / 2)

#define MAX_WALLS 9000

// bullet stats
#define MAX_MAP_BULLETS 600 // maximum bullet entities overall, dont change
#define MAX_BULLETS 10      // max bullets PER tank, change if wanted
#define MAX_BOUNCES 8
#define BULLET_R 6.0f // radius
#define BULLET_SPEED 840.0f
#define BULLET_LIFETIME 8.0f

// tank stats
#define START_HEALTH 3

#define TANK_W 60.0f
#define TANK_H 44.0f
#define TANK_SPEED 300.0f
#define TURN_SPEED 210.0f
#define CANNON_SCALE 0.5f

#define TANK_COLORS                                                            \
  {(Color){0, 228, 48, 255}, (Color){230, 41, 55, 255},                        \
   (Color){0, 121, 241, 255}, (Color){255, 161, 0, 255}};

// tank controls
#define TANK_CONTROLS_MSG                                                      \
  "P1 WASD + C ... "                                                           \
  "P2 IJKL + M ... "                                                           \
  "P3 Arrows + Space ... "                                                     \
  "P4 Numpad 8456 + 0 ... "                                                    \
  "R: new maze" // cosmetic only; real controls below

#define TANK_0_UP KEY_W
#define TANK_0_LEFT KEY_A
#define TANK_0_DOWN KEY_S
#define TANK_0_RIGHT KEY_D
#define TANK_0_FIRE KEY_C

#define TANK_1_UP KEY_I
#define TANK_1_LEFT KEY_J
#define TANK_1_DOWN KEY_K
#define TANK_1_RIGHT KEY_L
#define TANK_1_FIRE KEY_M

#define TANK_2_UP KEY_UP
#define TANK_2_LEFT KEY_LEFT
#define TANK_2_DOWN KEY_DOWN
#define TANK_2_RIGHT KEY_RIGHT
#define TANK_2_FIRE KEY_SPACE

#define TANK_3_UP KEY_KP_8
#define TANK_3_LEFT KEY_KP_4
#define TANK_3_DOWN KEY_KP_5
#define TANK_3_RIGHT KEY_KP_6
#define TANK_3_FIRE KEY_KP_0

// sound settings

#define MASTER_VOLUME 0.9f

// (advanced) checks to kills a bullet if it spawns inside a wall
// 0: allows wallshots if you go up to a wall
// 1: allows limited wallshots
// 2: recommended
// 3+: not really needed
#define WALL_SHOT_SAFETY_ITERS 2

#endif
