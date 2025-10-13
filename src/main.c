#include "raylib.h"
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

// tunables
#define SCREEN_W 1000
#define SCREEN_H 800
#define MAZE_COLS 25
#define MAZE_ROWS 18
#define CELL 40
#define WALL_T 8.0f

#define ORIGIN_X ((SCREEN_W - (MAZE_COLS * CELL)) / 2)
#define ORIGIN_Y ((SCREEN_H - (MAZE_ROWS * CELL)) / 2)

#define MAX_WALLS 8000
#define MAX_BULLETS 256
#define MAX_BOUNCES 6

#define BULLET_R 3.0f
#define BULLET_SPEED 420.0f
#define BULLET_LIFETIME 5.0f
#define TANK_W 28.0f
#define TANK_H 22.0f
#define TANK_SPEED 140.0f
#define TURN_SPEED 140.0f

// types (separate file?)
typedef struct {
  Vector2 pos;
  float angleDeg;
  Vector2 vel;
} Tank;

typedef struct {
  Vector2 pos, vel;
  bool active;
  float lifetimeSec;
  int bounces; // how many ricochets done
} Bullet;

typedef struct {
  Rectangle rect;
} Wall;

typedef struct {
  bool visited;
  bool wall[4]; // N,E,S,W
} Cell;

// globals
static Tank player;
static Bullet bullets[MAX_BULLETS];
static Wall walls[MAX_WALLS];
static int wallCount = 0;
static Cell maze[MAZE_ROWS][MAZE_COLS];

// func declarations
static void InitGame(void);
static void ResetRound(void);
static void UpdateGame(float dt);
static void DrawGame(void);

static void FireBullet(void);
static bool CheckBulletWallCollision(Bullet *b, Wall *w);
static void ReflectBullet(Bullet *b, Vector2 n);

static void MazeInit(void);
static void MazeDFS(int r, int c);
static void BuildWalls(void);

static void HandleTankWallCollision(Tank *t);

// math helpers
static inline float Dot2(Vector2 a, Vector2 b) { return a.x * b.x + a.y * b.y; }
static inline float Clampf(float x, float a, float b) {
  return x < a ? a : (x > b ? b : x);
}

int main(void) {
  InitWindow(SCREEN_W, SCREEN_H, "tank trouble raylib");
  SetTargetFPS(120);
  InitGame();

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();
    UpdateGame(dt);
    BeginDrawing();
    ClearBackground((Color){25, 25, 28, 255});
    DrawGame();
    EndDrawing();
  }
  CloseWindow();
  return 0;
}

static void InitGame(void) {
  SetRandomSeed((unsigned)time(NULL));
  ResetRound();
}

static void ResetRound(void) {
  MazeInit();
  MazeDFS(0, 0);
  BuildWalls();

  player.pos = (Vector2){ORIGIN_X + CELL / 2.0f, ORIGIN_Y + CELL / 2.0f};
  player.angleDeg = 0;
  player.vel = (Vector2){0, 0};
  for (int i = 0; i < MAX_BULLETS; i++)
    bullets[i].active = false;
}

static void UpdateGame(float dt) {
  // movement
  if (IsKeyDown(KEY_LEFT))
    player.angleDeg -= TURN_SPEED * dt;
  if (IsKeyDown(KEY_RIGHT))
    player.angleDeg += TURN_SPEED * dt;

  float rad = player.angleDeg * DEG2RAD;
  Vector2 fwd = {cosf(rad), sinf(rad)};

  if (IsKeyDown(KEY_UP)) {
    player.pos.x += fwd.x * TANK_SPEED * dt;
    player.pos.y += fwd.y * TANK_SPEED * dt;
  }
  if (IsKeyDown(KEY_DOWN)) {
    player.pos.x -= fwd.x * TANK_SPEED * dt;
    player.pos.y -= fwd.y * TANK_SPEED * dt;
  }

  HandleTankWallCollision(&player);

  if (IsKeyPressed(KEY_SPACE))
    FireBullet();
  if (IsKeyPressed(KEY_R))
    ResetRound();

  // bullet stuff
  for (int i = 0; i < MAX_BULLETS; i++) {
    Bullet *b = &bullets[i];
    if (!b->active)
      continue;
    b->pos.x += b->vel.x * dt;
    b->pos.y += b->vel.y * dt;
    b->lifetimeSec -= dt;

    bool bounced = false;
    for (int j = 0; j < wallCount; j++) {
      if (CheckBulletWallCollision(b, &walls[j])) {
        bounced = true;
        break;
      }
    }
    if (bounced) {
      b->bounces++;
      if (b->bounces > MAX_BOUNCES)
        b->active = false;
    }
    if (b->lifetimeSec <= 0)
      b->active = false;
  }
}

static void DrawGame(void) {
  // wall
  for (int i = 0; i < wallCount; i++)
    DrawRectangleRec(walls[i].rect, (Color){100, 100, 110, 255});

  // tank is rectangle oriented by angle
  Rectangle body = {player.pos.x, player.pos.y, TANK_W, TANK_H};
  DrawRectanglePro(body, (Vector2){TANK_W / 2, TANK_H / 2}, player.angleDeg,
                   (Color){30, 200, 30, 255});
  float rad = player.angleDeg * DEG2RAD;
  Vector2 barrel = {player.pos.x + cosf(rad) * TANK_W * 0.7f,
                    player.pos.y + sinf(rad) * TANK_W * 0.7f};
  DrawLineEx(player.pos, barrel, 4.0f, GREEN);

  // bullet
  for (int i = 0; i < MAX_BULLETS; i++)
    if (bullets[i].active)
      DrawCircleV(bullets[i].pos, BULLET_R, YELLOW);

  DrawText("Arrows move/turn  SPACE shoot  R regen maze", 10, 10, 18, RAYWHITE);
}

static void FireBullet(void) {
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (!bullets[i].active) {
      float rad = player.angleDeg * DEG2RAD;
      Vector2 dir = {cosf(rad), sinf(rad)};
      bullets[i].active = true;
      bullets[i].bounces = 0;
      bullets[i].lifetimeSec = BULLET_LIFETIME;
      bullets[i].pos = (Vector2){player.pos.x + dir.x * (TANK_W / 2 + 6),
                                 player.pos.y + dir.y * (TANK_W / 2 + 6)};
      bullets[i].vel = (Vector2){dir.x * BULLET_SPEED, dir.y * BULLET_SPEED};
      break;
    }
  }
}

static bool CheckBulletWallCollision(Bullet *b, Wall *w) {
  Rectangle bb = {b->pos.x - BULLET_R, b->pos.y - BULLET_R, BULLET_R * 2,
                  BULLET_R * 2};
  if (!CheckCollisionRecs(bb, w->rect))
    return false;

  float cx = b->pos.x, cy = b->pos.y;
  float left = cx - w->rect.x;
  float right = (w->rect.x + w->rect.width) - cx;
  float top = cy - w->rect.y;
  float bottom = (w->rect.y + w->rect.height) - cy;
  float minX = fminf(left, right), minY = fminf(top, bottom);
  Vector2 n = {0};
  if (minX < minY) {
    n = (left < right) ? (Vector2){-1, 0} : (Vector2){1, 0};
  } else {
    n = (top < bottom) ? (Vector2){0, -1} : (Vector2){0, 1};
  }
  ReflectBullet(b, n);
  b->pos.x += n.x * (WALL_T + BULLET_R);
  b->pos.y += n.y * (WALL_T + BULLET_R);
  return true;
}

static void ReflectBullet(Bullet *b, Vector2 n) {
  float len = sqrtf(n.x * n.x + n.y * n.y);
  if (len > 0) {
    n.x /= len;
    n.y /= len;
  }
  float d = Dot2(b->vel, n);
  b->vel.x -= 2 * d * n.x;
  b->vel.y -= 2 * d * n.y;
}

// axis aligned boudning boxes
static void HandleTankWallCollision(Tank *t) {
  Rectangle tankBox = {t->pos.x - TANK_W / 2, t->pos.y - TANK_H / 2, TANK_W,
                       TANK_H};

  for (int i = 0; i < wallCount; i++) {
    if (CheckCollisionRecs(tankBox, walls[i].rect)) {
      Rectangle w = walls[i].rect;
      float dx = (t->pos.x) - (w.x + w.width / 2);
      float px = (w.width / 2 + TANK_W / 2) - fabsf(dx);
      if (px <= 0)
        continue;

      float dy = (t->pos.y) - (w.y + w.height / 2);
      float py = (w.height / 2 + TANK_H / 2) - fabsf(dy);
      if (py <= 0)
        continue;

      if (px < py) {
        float sx = (dx < 0) ? -1 : 1;
        t->pos.x += px * sx;
      } else {
        float sy = (dy < 0) ? -1 : 1;
        t->pos.y += py * sy;
      }
      tankBox = (Rectangle){t->pos.x - TANK_W / 2, t->pos.y - TANK_H / 2,
                            TANK_W, TANK_H};
    }
  }
}

// basic dfs maze (see wikipedia)
static void MazeInit(void) {
  for (int r = 0; r < MAZE_ROWS; r++) {
    for (int c = 0; c < MAZE_COLS; c++) {
      maze[r][c].visited = false;
      maze[r][c].wall[0] = maze[r][c].wall[1] = maze[r][c].wall[2] =
          maze[r][c].wall[3] = true;
    }
  }
}
static void MazeDFS(int r, int c) {
  maze[r][c].visited = true;
  int dirs[4] = {0, 1, 2, 3};
  for (int i = 3; i > 0; i--) {
    int j = GetRandomValue(0, i);
    int tmp = dirs[i];
    dirs[i] = dirs[j];
    dirs[j] = tmp;
  }
  for (int k = 0; k < 4; k++) {
    int d = dirs[k];
    int nr = r, nc = c;
    if (d == 0)
      nr = r - 1;
    else if (d == 1)
      nc = c + 1;
    else if (d == 2)
      nr = r + 1;
    else
      nc = c - 1;
    if (nr < 0 || nr >= MAZE_ROWS || nc < 0 || nc >= MAZE_COLS)
      continue;
    if (maze[nr][nc].visited)
      continue;
    if (d == 0) {
      maze[r][c].wall[0] = false;
      maze[nr][nc].wall[2] = false;
    } else if (d == 1) {
      maze[r][c].wall[1] = false;
      maze[nr][nc].wall[3] = false;
    } else if (d == 2) {
      maze[r][c].wall[2] = false;
      maze[nr][nc].wall[0] = false;
    } else {
      maze[r][c].wall[3] = false;
      maze[nr][nc].wall[1] = false;
    }
    MazeDFS(nr, nc);
  }
}
static void BuildWalls(void) {
  wallCount = 0;
  // outer borders
  Rectangle top = {ORIGIN_X - WALL_T / 2, ORIGIN_Y - WALL_T / 2,
                   MAZE_COLS * CELL + WALL_T, WALL_T};
  walls[wallCount++].rect = top;
  Rectangle bottom = {ORIGIN_X - WALL_T / 2,
                      ORIGIN_Y + MAZE_ROWS * CELL - WALL_T / 2,
                      MAZE_COLS * CELL + WALL_T, WALL_T};
  walls[wallCount++].rect = bottom;
  Rectangle left = {ORIGIN_X - WALL_T / 2, ORIGIN_Y - WALL_T / 2, WALL_T,
                    MAZE_ROWS * CELL + WALL_T};
  walls[wallCount++].rect = left;
  Rectangle right = {ORIGIN_X + MAZE_COLS * CELL - WALL_T / 2,
                     ORIGIN_Y - WALL_T / 2, WALL_T, MAZE_ROWS * CELL + WALL_T};
  walls[wallCount++].rect = right;

  for (int r = 0; r < MAZE_ROWS; r++) {
    for (int c = 0; c < MAZE_COLS; c++) {
      if (maze[r][c].wall[1]) { // east
        float x = ORIGIN_X + (c + 1) * CELL;
        float y = ORIGIN_Y + r * CELL;
        Rectangle rec = {x - WALL_T / 2, y - WALL_T / 2, WALL_T, CELL + WALL_T};
        walls[wallCount++].rect = rec;
      }
      if (maze[r][c].wall[2]) { // south
        float x = ORIGIN_X + c * CELL;
        float y = ORIGIN_Y + (r + 1) * CELL;
        Rectangle rec = {x - WALL_T / 2, y - WALL_T / 2, CELL + WALL_T, WALL_T};
        walls[wallCount++].rect = rec;
      }
    }
  }
}
