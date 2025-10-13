#include "raylib.h"
#include <math.h>

#define MAX_BULLETS 64
// TODO: add more tunables

typedef struct {
  Vector2 pos;
  float angleDeg;
  float speed;
} Tank;

typedef struct {
  Vector2 pos;
  Vector2 vel;
  bool active;
  float lifetimeSec;
} Bullet;

static Tank player;
static Bullet bullets[MAX_BULLETS];
static const float bullet_speed = 400.0f;
static const float bullet_lifetime = 3.0f;
static const float tank_speed = 120.0f;
static const float turn_speed = 120.0f; // deg/s

void InitGame(void);
void UpdateGame(float dt);
void DrawGame(void);
void FireBullet(void);

int main(void) {
  const int screenWidth = 800;
  const int screenHeight = 600;

  InitWindow(screenWidth, screenHeight, "tank trouble raylib");
  SetTargetFPS(120);

  InitGame();

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();
    UpdateGame(dt);

    BeginDrawing();
    ClearBackground((Color){30, 30, 30, 255});
    DrawGame();
    EndDrawing();
  }

  CloseWindow();
  return 0;
}

void InitGame(void) {
  player.pos = (Vector2){400, 300};
  player.angleDeg = 0;
  player.speed = 0;
  for (int i = 0; i < MAX_BULLETS; i++)
    bullets[i].active = false;
}

void UpdateGame(float dt) {
  // tank moving
  if (IsKeyDown(KEY_LEFT))
    player.angleDeg -= turn_speed * dt;
  if (IsKeyDown(KEY_RIGHT))
    player.angleDeg += turn_speed * dt;

  float dir_x = cosf(player.angleDeg * DEG2RAD);
  float dir_y = sinf(player.angleDeg * DEG2RAD);

  if (IsKeyDown(KEY_UP)) {
    player.pos.x += dir_x * tank_speed * dt;
    player.pos.y += dir_y * tank_speed * dt;
  }
  if (IsKeyDown(KEY_DOWN)) {
    player.pos.x -= dir_x * tank_speed * dt;
    player.pos.y -= dir_y * tank_speed * dt;
  }

  // bullet stuff
  if (IsKeyPressed(KEY_SPACE))
    FireBullet();

  for (int i = 0; i < MAX_BULLETS; i++) {
    if (!bullets[i].active)
      continue;

    bullets[i].pos.x += bullets[i].vel.x * dt;
    bullets[i].pos.y += bullets[i].vel.y * dt;
    bullets[i].lifetimeSec -= dt;

    // TODO: wall reflection
    if (bullets[i].lifetimeSec <= 0)
      bullets[i].active = false;

    if (bullets[i].pos.x < 0 || bullets[i].pos.x > GetScreenWidth() ||
        bullets[i].pos.y < 0 || bullets[i].pos.y > GetScreenHeight()) {
      bullets[i].active = false;
    }
    // TODO: basic ui, at least controls at minimum
  }
}

void DrawGame(void) {
  float rad = player.angleDeg * DEG2RAD;
  Vector2 barrel = {player.pos.x + cosf(rad) * 20,
                    player.pos.y + sinf(rad) * 20};
  DrawCircleV(player.pos, 15, DARKGREEN);
  DrawLineV(player.pos, barrel, GREEN);
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (bullets[i].active)
      DrawCircleV(bullets[i].pos, 3, YELLOW);
  }
}

void FireBullet(void) {
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (!bullets[i].active) {
      bullets[i].active = true;
      bullets[i].lifetimeSec = bullet_lifetime;

      float rad = player.angleDeg * DEG2RAD;
      bullets[i].pos = (Vector2){player.pos.x + cosf(rad) * 25,
                                 player.pos.y + sinf(rad) * 25};
      bullets[i].vel =
          (Vector2){cosf(rad) * bullet_speed, sinf(rad) * bullet_speed};
      break;
    }
  }
}
