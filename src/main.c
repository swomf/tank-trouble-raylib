#include "config.h"
#include "types.h"
#include <math.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

// globals
// static Tank tanks[TOTAL_TANKS]; // defined in config.h
static Bullet bullets[MAX_MAP_BULLETS];
static Wall walls[MAX_WALLS];
static int wallCount = 0;
static Cell maze[MAZE_ROWS][MAZE_COLS];
static Sound SND_FIRE;

// helpers
static inline float Dot2(Vector2 a, Vector2 b) { return a.x * b.x + a.y * b.y; }
static inline float Len2(Vector2 a) { return sqrtf(a.x * a.x + a.y * a.y); }

// func declarations
static void InitGame(void);
static void ResetRound(void);
static void UpdateGame(float dt);
static void DrawGame(void);

static void FireBullet(int tidx);
static bool BulletCollideWall(Bullet *b, Wall *w);
static void ReflectBullet(Bullet *b, Vector2 n);
static void HandleTankWallCollision(Tank *t);
static void deactivateBullet(Bullet *b);

static void MazeInit(void);
static void MazeDFS(int r, int c);
static void MazeAddLoops(void);
static void BuildWalls(void);

static bool BulletHitsTank(Bullet *b, const Tank *t);
static void HandleBulletHitsTanks(void);
static void HandleBulletHitsBullets(void);

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
  UnloadSound(SND_FIRE);
  CloseAudioDevice();
  CloseWindow();
  return 0;
}

static void InitGame(void) {
  InitAudioDevice();
  SetMasterVolume(MASTER_VOLUME);
  SND_FIRE = LoadSound("assets/fire.ogg");
  SetSoundVolume(SND_FIRE, 0.55f);

  SetRandomSeed((unsigned)time(NULL));
  ResetRound();
}

static void ResetRound(void) {
  MazeInit();
  MazeDFS(0, 0);
  MazeAddLoops(); // slice out some walls from dfs to make maze open
  BuildWalls();

  // corners respawn

  for (int i = 0; i < TOTAL_TANKS; i++) {
    tanks[i].pos = tanks[i].spawnpoint;
    tanks[i].angleDeg = tanks[i].startAngleDeg;
    tanks[i].health = tanks[i].startHealth;
    tanks[i].alive = true;
  }
  for (int i = 0; i < MAX_MAP_BULLETS; i++)
    if (bullets[i].active)
      deactivateBullet(&bullets[i]);
}

static void UpdateGame(float dt) {
  for (int i = 0; i < TOTAL_TANKS; i++) {
    if (!tanks[i].alive)
      continue;
    if (IsKeyDown(tanks[i].left))
      tanks[i].angleDeg -= TURN_SPEED * dt;
    if (IsKeyDown(tanks[i].right))
      tanks[i].angleDeg += TURN_SPEED * dt;

    float r = tanks[i].angleDeg * DEG2RAD;
    Vector2 f = (Vector2){cosf(r), sinf(r)};
    if (IsKeyDown(tanks[i].up)) {
      tanks[i].pos.x += f.x * TANK_SPEED * dt;
      tanks[i].pos.y += f.y * TANK_SPEED * dt;
    }
    if (IsKeyDown(tanks[i].down)) {
      tanks[i].pos.x -= f.x * TANK_SPEED * dt;
      tanks[i].pos.y -= f.y * TANK_SPEED * dt;
    }
    HandleTankWallCollision(&tanks[i]);
    if (IsKeyPressed(tanks[i].fire))
      FireBullet(i);
  }

  if (IsKeyPressed(KEY_R)) {
    // R resets round, prevent R from being misfired before round ends
    if (SAFE_R) {
      int j = 0;
      for (int i = 0; i < sizeof(tanks) / sizeof(Tank); i++) {
        if (tanks[i].alive)
          j++;
      }
      if (j <= 1)
        ResetRound();
    } else {
      ResetRound();
    }
  }

  // bullet stuff
  for (int i = 0; i < MAX_MAP_BULLETS; i++) {
    Bullet *b = &bullets[i];
    if (!b->active)
      continue;
    b->pos.x += b->vel.x * dt;
    b->pos.y += b->vel.y * dt;
    b->lifetimeSec -= dt;

    bool bounced = false;
    for (int j = 0; j < wallCount; j++) {
      if (BulletCollideWall(b, &walls[j])) {
        bounced = true;
        break;
      }
    }
    if (bounced) {
      b->bounces++;
      if (b->bounces > MAX_BOUNCES)
        deactivateBullet(b);
    }
    if (b->lifetimeSec <= 0)
      deactivateBullet(b);
    if (b->pos.x < -200 || b->pos.x > SCREEN_W + 200 || b->pos.y < -200 ||
        b->pos.y > SCREEN_H + 200)
      deactivateBullet(b);
  }

  // pvp. also bullets can hit each other
  HandleBulletHitsTanks();
  HandleBulletHitsBullets();
}

static void DrawGame(void) {
  for (int i = 0; i < wallCount; i++)
    DrawRectangleRec(walls[i].rect, (Color){100, 100, 110, 255});

  for (int t = 0; t < 4; t++) {
    if (!tanks[t].alive)
      continue;
    float rad = tanks[t].angleDeg * DEG2RAD;
    Rectangle body = {tanks[t].pos.x, tanks[t].pos.y, TANK_W, TANK_H};
    DrawRectanglePro(body, (Vector2){TANK_W / 2, TANK_H / 2}, tanks[t].angleDeg,
                     tanks[t].color);
    // TODO: barrel length is visual only. basically legacy code
    Vector2 barrel = {tanks[t].pos.x + cosf(rad) * TANK_W * 0.5f,
                      tanks[t].pos.y + sinf(rad) * TANK_W * 0.5f};
    DrawLineEx(tanks[t].pos, barrel, 4.0f, RAYWHITE);
  }

  // bullet
  for (int i = 0; i < MAX_MAP_BULLETS; i++)
    if (bullets[i].active)
      DrawCircleV(bullets[i].pos, BULLET_R, bullets[i].color);

  int y = 10;
  DrawText(TANK_CONTROLS_MSG, 10, y, 18, RAYWHITE);
  y += 22;
  for (int t = 0; t < 4; t++) {
    DrawText(TextFormat("P%d health: %d", t + 1, tanks[t].health), 10 + t * 220,
             y, 20, tanks[t].color);
  }
}

// for bullet destruction see "bullet stuff" in UpdateGame
//
// also see all deactivateBullet(Bullet* b) calls
static void FireBullet(int tidx) {
  if (!tanks[tidx].alive || tanks[tidx].bulletsActive >= MAX_BULLETS)
    return;

  for (int i = 0; i < MAX_MAP_BULLETS; i++) {
    if (bullets[i].active) // find first inactive bullet to animate
      continue;

    float rad = tanks[tidx].angleDeg * DEG2RAD;
    Vector2 dir = (Vector2){cosf(rad), sinf(rad)};

    tanks[tidx].bulletsActive++;

    bullets[i].active = true;
    bullets[i].bounces = 0;
    bullets[i].lifetimeSec = BULLET_LIFETIME;
    bullets[i].color = tanks[tidx].color;
    bullets[i].pos = (Vector2){
        tanks[tidx].pos.x + dir.x * (TANK_W * 0.5f + BULLET_R + 2.0f),
        tanks[tidx].pos.y + dir.y * (TANK_W * 0.5f + BULLET_R + 2.0f)};
    bullets[i].vel = (Vector2){dir.x * BULLET_SPEED, dir.y * BULLET_SPEED};

    PlaySound(SND_FIRE); // sounds can't overlap though

    // Bullet spawn safety: if spawned overlapping a wall (barrel stuffed),
    // annihilate it
    for (int pass = 0; pass < WALL_SHOT_SAFETY_ITERS; pass++) {
      bool hit = false;
      for (int w = 0; w < wallCount; w++) {
        if (BulletCollideWall(&bullets[i], &walls[w])) {
          hit = true;
          break;
        }
      }
      if (!hit)
        break;
      bullets[i].bounces = MAX_BOUNCES;
    }
    return;
  }
}

static void deactivateBullet(Bullet *b) {
  b->active = false;
  for (int i = 0; i < sizeof(tanks) / sizeof(Tank); i++) {
    if (tanks[i].color.r == b->color.r && tanks[i].color.g == b->color.g &&
        tanks[i].color.b == b->color.b)
      tanks[i].bulletsActive--;
  }
}

static bool BulletCollideWall(Bullet *b, Wall *w) {
  Rectangle bb = {b->pos.x - BULLET_R, b->pos.y - BULLET_R, BULLET_R * 2,
                  BULLET_R * 2};
  if (!CheckCollisionRecs(bb, w->rect))
    return false;

  float cx = b->pos.x, cy = b->pos.y;
  float left = cx - w->rect.x;
  float right = (w->rect.x + w->rect.width) - cx;
  float top = cy - w->rect.y;
  float bottom = (w->rect.y + w->rect.height) - cy;

  float minX = fminf(left, right);
  float minY = fminf(top, bottom);

  Vector2 n = {0};
  if (minX < minY)
    n = (left < right) ? (Vector2){-1, 0} : (Vector2){1, 0};
  else
    n = (top < bottom) ? (Vector2){0, -1} : (Vector2){0, 1};

  ReflectBullet(b, n);

  const float sep = WALL_T * 0.6f + BULLET_R;
  b->pos.x += n.x * sep;
  b->pos.y += n.y * sep;

  return true;
}

static void ReflectBullet(Bullet *b, Vector2 n) {
  float L = Len2(n);
  if (L > 0) {
    n.x /= L;
    n.y /= L;
  }
  float d = Dot2(b->vel, n);
  b->vel.x -= 2.0f * d * n.x;
  b->vel.y -= 2.0f * d * n.y;
}

// axis aligned boudning boxes
static void HandleTankWallCollision(Tank *t) {
  Rectangle tankBox = {t->pos.x - TANK_W / 2, t->pos.y - TANK_W / 2, TANK_W,
                       TANK_W};

  for (int i = 0; i < wallCount; i++) {
    if (CheckCollisionRecs(tankBox, walls[i].rect)) {
      Rectangle w = walls[i].rect;
      float dx = (t->pos.x) - (w.x + w.width / 2);
      float px = (w.width / 2 + TANK_W / 2) - fabsf(dx);
      if (px <= 0)
        continue;

      float dy = (t->pos.y) - (w.y + w.height / 2);
      float py = (w.height / 2 + TANK_W / 2) - fabsf(dy);
      if (py <= 0)
        continue;

      if (px < py) {
        t->pos.x += (dx < 0 ? -px : px);
      } else {
        t->pos.y += (dy < 0 ? -py : py);
      }
    }
  }
}

// bullet vs tank is approximated by circle. dont tell the tryhards
// selfdamage allowed ofc
static bool BulletHitsTank(Bullet *b, const Tank *t) {
  if (!t->alive)
    return false;
  float rr = (TANK_W > TANK_H ? TANK_W : TANK_H) * 0.5f; // radius
  float dx = b->pos.x - t->pos.x;
  float dy = b->pos.y - t->pos.y;
  float r = rr + BULLET_R;
  return (dx * dx + dy * dy) <= (r * r);
}

static void HandleBulletHitsTanks(void) {
  for (int i = 0; i < MAX_MAP_BULLETS; i++) {
    Bullet *b = &bullets[i];
    if (!b->active)
      continue;
    for (int t = 0; t < TOTAL_TANKS; t++) {
      if (BulletHitsTank(b, &tanks[t])) {
        deactivateBullet(b);
        if (tanks[t].alive) {
          tanks[t].health--;
          tanks[t].alive = (tanks[t].health > 0);
        }
        break;
      }
    }
  }
}

// bullet vs bullet is obviously circle vs circle
// they destroy each other on impact because i dont trust ngnl style bouncing
// (yet...)
static void HandleBulletHitsBullets(void) {
  for (int i = 0; i < MAX_MAP_BULLETS; i++) {
    if (!bullets[i].active)
      continue;
    for (int j = i + 1; j < MAX_MAP_BULLETS; j++) {
      if (!bullets[j].active)
        continue;
      float dx = bullets[i].pos.x - bullets[j].pos.x;
      float dy = bullets[i].pos.y - bullets[j].pos.y;
      float rr = (BULLET_R + BULLET_R);
      if (dx * dx + dy * dy <= rr * rr) {
        deactivateBullet(&bullets[i]);
        deactivateBullet(&bullets[j]);
        break;
      }
    }
  }
}

// basic dfs maze (see wikipedia)
// to make imperfect (has loops) we just randomly remove a few walls
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
  int dirs[4] = {0, 1, 2, 3}; // n,e,s,w
  for (int i = 3; i > 0; i--) {
    int j = GetRandomValue(0, i);
    int t = dirs[i];
    dirs[i] = dirs[j];
    dirs[j] = t;
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
static void MazeAddLoops(void) {
  for (int r = 0; r < MAZE_ROWS; r++) {
    for (int c = 0; c < MAZE_COLS; c++) {
      // east wall (avoid outer boundary)
      if (c < MAZE_COLS - 1 && maze[r][c].wall[1] &&
          GetRandomValue(0, 1000) < (int)(LOOP_CHANCE * 1000)) {
        maze[r][c].wall[1] = false;
        maze[r][c + 1].wall[3] = false;
      }
      // south wall
      if (r < MAZE_ROWS - 1 && maze[r][c].wall[2] &&
          GetRandomValue(0, 1000) < (int)(LOOP_CHANCE * 1000)) {
        maze[r][c].wall[2] = false;
        maze[r + 1][c].wall[0] = false;
      }
    }
  }
}

static void BuildWalls(void) {
  wallCount = 0;
  // outer borders
  walls[wallCount++].rect =
      (Rectangle){ORIGIN_X - WALL_T / 2, ORIGIN_Y - WALL_T / 2,
                  MAZE_COLS * CELL + WALL_T, WALL_T};
  walls[wallCount++].rect = (Rectangle){
      ORIGIN_X - WALL_T / 2, ORIGIN_Y + MAZE_ROWS * CELL - WALL_T / 2,
      MAZE_COLS * CELL + WALL_T, WALL_T};
  walls[wallCount++].rect =
      (Rectangle){ORIGIN_X - WALL_T / 2, ORIGIN_Y - WALL_T / 2, WALL_T,
                  MAZE_ROWS * CELL + WALL_T};
  walls[wallCount++].rect =
      (Rectangle){ORIGIN_X + MAZE_COLS * CELL - WALL_T / 2,
                  ORIGIN_Y - WALL_T / 2, WALL_T, MAZE_ROWS * CELL + WALL_T};

  // internal walls: only add east and south to avoid dupes
  for (int r = 0; r < MAZE_ROWS; r++) {
    for (int c = 0; c < MAZE_COLS; c++) {
      if (maze[r][c].wall[1]) { // east
        float x = ORIGIN_X + (c + 1) * CELL;
        float y = ORIGIN_Y + r * CELL;
        if (wallCount < MAX_WALLS)
          walls[wallCount++].rect = (Rectangle){x - WALL_T / 2, y - WALL_T / 2,
                                                WALL_T, CELL + WALL_T};
      }
      if (maze[r][c].wall[2]) { // south
        float x = ORIGIN_X + c * CELL;
        float y = ORIGIN_Y + (r + 1) * CELL;
        if (wallCount < MAX_WALLS)
          walls[wallCount++].rect = (Rectangle){x - WALL_T / 2, y - WALL_T / 2,
                                                CELL + WALL_T, WALL_T};
      }
    }
  }
}
