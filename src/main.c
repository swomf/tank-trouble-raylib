#include "config.h"
#include <math.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

// types
typedef struct {
  Vector2 pos;
  float angleDeg;
  Color color;
  int health;
  bool alive;
} Tank;

typedef struct {
  Vector2 pos, vel;
  bool active;
  float lifetimeSec;
  int bounces;
  int owner; // tank index [0..3]
  Color color;
} Bullet;

typedef struct {
  Rectangle rect;
} Wall;

typedef struct {
  bool visited;
  bool wall[4]; // N,E,S,W
} Cell;

// globals
static Tank tanks[4];
static Bullet bullets[MAX_BULLETS];
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
  Vector2 corners[4] = {
      (Vector2){ORIGIN_X + CELL * 0.5f, ORIGIN_Y + CELL * 0.5f},
      (Vector2){ORIGIN_X + (MAZE_COLS - 1) * CELL + CELL * 0.5f,
                ORIGIN_Y + CELL * 0.5f},
      (Vector2){ORIGIN_X + CELL * 0.5f,
                ORIGIN_Y + (MAZE_ROWS - 1) * CELL + CELL * 0.5f},
      (Vector2){ORIGIN_X + (MAZE_COLS - 1) * CELL + CELL * 0.5f,
                ORIGIN_Y + (MAZE_ROWS - 1) * CELL + CELL * 0.5f}};
  Color colors[4] = TANK_COLORS;

  for (int i = 0; i < 4; i++) {
    tanks[i].pos = corners[i];
    tanks[i].angleDeg = (i == 0)   ? 45.0f    // top left green
                        : (i == 1) ? 135.0f   // top right red
                        : (i == 2) ? -45.0f   // bottoml eft blue
                                   : -135.0f; // bottom right yellow
    tanks[i].color = colors[i];
    tanks[i].health = START_HEALTH;
    tanks[i].alive = true;
  }
  for (int i = 0; i < MAX_BULLETS; i++)
    bullets[i].active = false;
}

static void UpdateGame(float dt) {
  // P1 controls
  if (tanks[0].alive) {
    if (IsKeyDown(TANK_0_LEFT))
      tanks[0].angleDeg -= TURN_SPEED * dt;
    if (IsKeyDown(TANK_0_RIGHT))
      tanks[0].angleDeg += TURN_SPEED * dt;
    float r = tanks[0].angleDeg * DEG2RAD;
    Vector2 f = (Vector2){cosf(r), sinf(r)};
    if (IsKeyDown(TANK_0_UP)) {
      tanks[0].pos.x += f.x * TANK_SPEED * dt;
      tanks[0].pos.y += f.y * TANK_SPEED * dt;
    }
    if (IsKeyDown(TANK_0_DOWN)) {
      tanks[0].pos.x -= f.x * TANK_SPEED * dt;
      tanks[0].pos.y -= f.y * TANK_SPEED * dt;
    }
    HandleTankWallCollision(&tanks[0]);
    if (IsKeyPressed(KEY_C))
      FireBullet(0);
  }

  // P2 controls
  if (tanks[1].alive) {
    if (IsKeyDown(TANK_1_LEFT))
      tanks[1].angleDeg -= TURN_SPEED * dt;
    if (IsKeyDown(TANK_1_RIGHT))
      tanks[1].angleDeg += TURN_SPEED * dt;
    float r = tanks[1].angleDeg * DEG2RAD;
    Vector2 f = (Vector2){cosf(r), sinf(r)};
    if (IsKeyDown(TANK_1_UP)) {
      tanks[1].pos.x += f.x * TANK_SPEED * dt;
      tanks[1].pos.y += f.y * TANK_SPEED * dt;
    }
    if (IsKeyDown(TANK_1_DOWN)) {
      tanks[1].pos.x -= f.x * TANK_SPEED * dt;
      tanks[1].pos.y -= f.y * TANK_SPEED * dt;
    }
    HandleTankWallCollision(&tanks[1]);
    if (IsKeyPressed(KEY_M))
      FireBullet(1);
  }

  // P3 controls
  if (tanks[2].alive) {
    if (IsKeyDown(TANK_2_LEFT))
      tanks[2].angleDeg -= TURN_SPEED * dt;
    if (IsKeyDown(TANK_2_RIGHT))
      tanks[2].angleDeg += TURN_SPEED * dt;
    float r = tanks[2].angleDeg * DEG2RAD;
    Vector2 f = (Vector2){cosf(r), sinf(r)};
    if (IsKeyDown(TANK_2_UP)) {
      tanks[2].pos.x += f.x * TANK_SPEED * dt;
      tanks[2].pos.y += f.y * TANK_SPEED * dt;
    }
    if (IsKeyDown(TANK_2_DOWN)) {
      tanks[2].pos.x -= f.x * TANK_SPEED * dt;
      tanks[2].pos.y -= f.y * TANK_SPEED * dt;
    }
    HandleTankWallCollision(&tanks[2]);
    if (IsKeyPressed(KEY_SPACE))
      FireBullet(2);
  }

  // P4 controls
  if (tanks[3].alive) {
    if (IsKeyDown(TANK_3_LEFT))
      tanks[3].angleDeg -= TURN_SPEED * dt;
    if (IsKeyDown(TANK_3_RIGHT))
      tanks[3].angleDeg += TURN_SPEED * dt;
    float r = tanks[3].angleDeg * DEG2RAD;
    Vector2 f = (Vector2){cosf(r), sinf(r)};
    if (IsKeyDown(TANK_3_UP)) {
      tanks[3].pos.x += f.x * TANK_SPEED * dt;
      tanks[3].pos.y += f.y * TANK_SPEED * dt;
    }
    if (IsKeyDown(TANK_3_DOWN)) {
      tanks[3].pos.x -= f.x * TANK_SPEED * dt;
      tanks[3].pos.y -= f.y * TANK_SPEED * dt;
    }
    HandleTankWallCollision(&tanks[3]);
    if (IsKeyPressed(TANK_3_FIRE))
      FireBullet(3);
  }

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
      if (BulletCollideWall(b, &walls[j])) {
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
    if (b->pos.x < -200 || b->pos.x > SCREEN_W + 200 || b->pos.y < -200 ||
        b->pos.y > SCREEN_H + 200)
      b->active = false;
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
    Vector2 barrel = {tanks[t].pos.x + cosf(rad) * TANK_W * CANNON_SCALE,
                      tanks[t].pos.y + sinf(rad) * TANK_W * CANNON_SCALE};
    DrawLineEx(tanks[t].pos, barrel, 4.0f, RAYWHITE);
  }

  // bullet
  for (int i = 0; i < MAX_BULLETS; i++)
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

static void FireBullet(int tidx) {
  if (!tanks[tidx].alive)
    return;

  for (int i = 0; i < MAX_BULLETS; i++) {
    if (!bullets[i].active) {
      float rad = tanks[tidx].angleDeg * DEG2RAD;
      Vector2 dir = (Vector2){cosf(rad), sinf(rad)};

      bullets[i].active = true;
      bullets[i].bounces = 0;
      bullets[i].lifetimeSec = BULLET_LIFETIME;
      bullets[i].owner = tidx;
      bullets[i].color = tanks[tidx].color;
      bullets[i].pos = (Vector2){
          tanks[tidx].pos.x + dir.x * (TANK_W * 0.5f + BULLET_R + 2.0f),
          tanks[tidx].pos.y + dir.y * (TANK_W * 0.5f + BULLET_R + 2.0f)};
      bullets[i].vel = (Vector2){dir.x * BULLET_SPEED, dir.y * BULLET_SPEED};

      PlaySound(SND_FIRE); // sounds can't overlap though

      // Bullet spawn safety: if spawned overlapping a wall (barrel stuffed),
      // annihilate it (TODO: this should maybe just bounce back instead)
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
        t->pos.x += (dx < 0 ? -px : px);
      } else {
        t->pos.y += (dy < 0 ? -py : py);
      }

      tankBox = (Rectangle){t->pos.x - TANK_W / 2, t->pos.y - TANK_H / 2,
                            TANK_W, TANK_H};
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
  for (int i = 0; i < MAX_BULLETS; i++) {
    Bullet *b = &bullets[i];
    if (!b->active)
      continue;
    for (int t = 0; t < 4; t++) {
      if (BulletHitsTank(b, &tanks[t])) {
        b->active = false;
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
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (!bullets[i].active)
      continue;
    for (int j = i + 1; j < MAX_BULLETS; j++) {
      if (!bullets[j].active)
        continue;
      float dx = bullets[i].pos.x - bullets[j].pos.x;
      float dy = bullets[i].pos.y - bullets[j].pos.y;
      float rr = (BULLET_R + BULLET_R);
      if (dx * dx + dy * dy <= rr * rr) {
        bullets[i].active = false;
        bullets[j].active = false;
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
