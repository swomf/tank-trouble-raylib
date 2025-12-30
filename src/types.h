#ifndef TYPES_H
#define TYPES_H
#include <raylib.h>

typedef struct {
  Vector2 pos;
  float angleDeg;
  int health;
  bool alive;
  int bulletsActive;

  // consts defined within config.h
  const int up;
  const int left;
  const int down;
  const int right;
  const int fire;

  const Color color;
  const int startHealth;

  const Vector2 spawnpoint;
  const float startAngleDeg;
} Tank;

typedef struct {
  Vector2 pos, vel;
  bool active;
  float lifetimeSec;
  int bounces;
  Color color;
} Bullet;

typedef struct {
  Rectangle rect;
} Wall;

typedef struct {
  bool visited;
  bool wall[4]; // N,E,S,W
} Cell;

#endif
