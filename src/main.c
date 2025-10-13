#include "raylib.h"
#include <math.h>

#define MAX_BULLETS 64
#define MAX_WALLS 16
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
  int bounces; // how many ricochets done
} Bullet;

typedef struct {
  Rectangle rect;
} Wall;

static Tank player;
static Bullet bullets[MAX_BULLETS];
static Wall walls[MAX_WALLS];
static int wallCount = 0;

// also tunables
static const float bullet_speed = 400.0f;
static const float bullet_lifetime = 5.0f; // seconds
static const float tank_speed = 120.0f;
static const float turn_speed = 120.0f; // degrees per second
static const int max_bounces = 6;

void InitGame(void);
void UpdateGame(float dt);
void DrawGame(void);
void FireBullet(void);
bool CheckBulletWallCollision(Bullet *b, Wall *w);
void ReflectBullet(Bullet *b, Vector2 normal);

int main(void) {
  // also tunables...
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

  // walls
  wallCount = 0;
  walls[wallCount++].rect = (Rectangle){100, 100, 600, 10}; // top h
  walls[wallCount++].rect = (Rectangle){100, 490, 600, 10}; // bottom h
  walls[wallCount++].rect = (Rectangle){100, 100, 10, 400}; // left v
  walls[wallCount++].rect = (Rectangle){690, 100, 10, 400}; // right v
  walls[wallCount++].rect = (Rectangle){350, 250, 100, 10}; // internal h
  walls[wallCount++].rect = (Rectangle){500, 350, 10, 100}; // internal v
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
    Bullet *b = &bullets[i];
    if (!b->active)
      continue;

    b->pos.x += b->vel.x * dt;
    b->pos.y += b->vel.y * dt;
    b->lifetimeSec -= dt;

    bool bounced = false;
    Vector2 normal = {0};

    for (int j = 0; j < wallCount; j++) {
      if (CheckBulletWallCollision(b, &walls[j])) {
        bounced = true;
        break;
      }
    }

    if (bounced) {
      b->bounces++;
      if (b->bounces > max_bounces)
        b->active = false;
    }

    if (b->lifetimeSec <= 0)
      b->active = false;
  }
}

void DrawGame(void) {
  // wall
  for (int i = 0; i < wallCount; i++) {
    DrawRectangleRec(walls[i].rect, GRAY);
  }

  // tank
  float rad = player.angleDeg * DEG2RAD;
  Vector2 barrel = {player.pos.x + cosf(rad) * 20,
                    player.pos.y + sinf(rad) * 20};
  DrawCircleV(player.pos, 15, DARKGREEN);
  DrawLineV(player.pos, barrel, GREEN);

  // bullet
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (bullets[i].active)
      DrawCircleV(bullets[i].pos, 3, YELLOW);
  }

  DrawText("arrow keys move, SPACE shoots. ricochets wip", 10, 10, 18,
           RAYWHITE);
}

void FireBullet(void) {
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (!bullets[i].active) {
      bullets[i].active = true;
      bullets[i].lifetimeSec = bullet_lifetime;
      bullets[i].bounces = 0;

      float rad = player.angleDeg * DEG2RAD;
      bullets[i].pos = (Vector2){player.pos.x + cosf(rad) * 25,
                                 player.pos.y + sinf(rad) * 25};
      bullets[i].vel =
          (Vector2){cosf(rad) * bullet_speed, sinf(rad) * bullet_speed};
      break;
    }
  }
}

// simple reflector for bullet velocity on wall collision
bool CheckBulletWallCollision(Bullet *b, Wall *w) {
  const float r = 3.0f; // bullet radius

  Rectangle rbox = {b->pos.x - r, b->pos.y - r, r * 2, r * 2};

  if (CheckCollisionRecs(rbox, w->rect)) {
    // Determine which side it hit using penetration depth
    float left = b->pos.x - w->rect.x;
    float right = (w->rect.x + w->rect.width) - b->pos.x;
    float top = b->pos.y - w->rect.y;
    float bottom = (w->rect.y + w->rect.height) - b->pos.y;

    float minX = fminf(left, right);
    float minY = fminf(top, bottom);

    Vector2 n = {0};

    if (minX < minY) {
      // collided with vertical wall
      n = (left < right) ? (Vector2){-1, 0} : (Vector2){1, 0};
    } else {
      // collided with horizontal wall
      n = (top < bottom) ? (Vector2){0, -1} : (Vector2){0, 1};
    }

    ReflectBullet(b, n);

    // move bullet by an epsilon so it doesnt stick (hopefully)
    b->pos.x += n.x * 2.0f;
    b->pos.y += n.y * 2.0f;

    return true;
  }
  return false;
}

// helper to reflect velocity across normal vector
void ReflectBullet(Bullet *b, Vector2 normal) {
  float dot = b->vel.x * normal.x + b->vel.y * normal.y;
  b->vel.x = b->vel.x - 2 * dot * normal.x;
  b->vel.y = b->vel.y - 2 * dot * normal.y;
}
