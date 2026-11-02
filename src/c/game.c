#include "game.h"
#include "settings.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define KEY_HIGH_SCORE 1
#define FP_SHIFT 8
#define FP_ONE (1 << FP_SHIFT)
#define HIT_COOLDOWN_PEGS 3

static Game s_game;
static AppTimer *s_timer = NULL;
static int8_t s_recent_pegs[HIT_COOLDOWN_PEGS];
static uint8_t s_recent_idx;

static void timer_cb(void *ctx);

static void vibe_short(void) {
  if (settings_get()->vibrate) vibes_short_pulse();
}

static void vibe_double(void) {
  if (settings_get()->vibrate) vibes_double_pulse();
}

static void schedule_tick(void) {
  s_timer = app_timer_register(GAME_TICK_MS, timer_cb, NULL);
}

static void set_state(GameState st, const char *banner) {
  s_game.state = st;
  s_game.state_ms = 0;
  if (banner) {
    strncpy(s_game.banner, banner, sizeof(s_game.banner) - 1);
    s_game.banner[sizeof(s_game.banner) - 1] = '\0';
  } else {
    s_game.banner[0] = '\0';
  }
}

static void save_high(void) {
  if (s_game.score > s_game.high_score) {
    s_game.high_score = s_game.score;
    persist_write_int(KEY_HIGH_SCORE, (int)s_game.high_score);
  }
}

static void clear_recent_pegs(void) {
  for (uint8_t i = 0; i < HIT_COOLDOWN_PEGS; i++) s_recent_pegs[i] = -1;
  s_recent_idx = 0;
}

static bool peg_recent(int8_t idx) {
  for (uint8_t i = 0; i < HIT_COOLDOWN_PEGS; i++) {
    if (s_recent_pegs[i] == idx) return true;
  }
  return false;
}

static void mark_peg(int8_t idx) {
  s_recent_pegs[s_recent_idx] = idx;
  s_recent_idx = (uint8_t)((s_recent_idx + 1) % HIT_COOLDOWN_PEGS);
}

static void place_aim_ball(void) {
  s_game.ball.x = ((int32_t)s_game.aim_x) << FP_SHIFT;
  s_game.ball.y = ((int32_t)s_game.board.drop_y) << FP_SHIFT;
  s_game.ball.vx = 0;
  s_game.ball.vy = 0;
  s_game.ball.last_peg = -1;
  clear_recent_pegs();
}

static void clamp_aim(void) {
  const Board *b = &s_game.board;
  int16_t min_x = (int16_t)(b->left + BOARD_BALL_R + 2);
  int16_t max_x = (int16_t)(b->right - BOARD_BALL_R - 2);
  if (s_game.aim_x < min_x) s_game.aim_x = min_x;
  if (s_game.aim_x > max_x) s_game.aim_x = max_x;
}

static void begin_aim(void) {
  place_aim_ball();
  set_state(GameState_Aim, "AIM  SELECT: drop");
}

static void drop_ball(void) {
  clear_recent_pegs();
  s_game.ball.x = ((int32_t)s_game.aim_x) << FP_SHIFT;
  s_game.ball.y = ((int32_t)s_game.board.drop_y) << FP_SHIFT;
  s_game.ball.vx = (int32_t)((rand() % 41) - 20); // tiny initial jitter
  s_game.ball.vy = 40;
  s_game.ball.last_peg = -1;
  s_game.last_points = 0;
  s_game.last_bin = -1;
  set_state(GameState_Falling, NULL);
}

static void score_ball(int8_t bin_idx) {
  const Bin *bin = &s_game.board.bins[bin_idx];
  s_game.last_bin = bin_idx;
  s_game.last_points = bin->multiplier;
  s_game.score = (uint16_t)(s_game.score + bin->multiplier);
  save_high();

  if (s_game.balls_left > 0) s_game.balls_left--;

  char msg[28];
  snprintf(msg, sizeof(msg), "+%d", (int)bin->multiplier);
  set_state(GameState_Scored, msg);

  if (bin->multiplier >= 10) {
    vibe_double();
  } else {
    vibe_short();
  }
}

static void resolve_peg_hit(int8_t peg_idx) {
  if (peg_recent(peg_idx)) return;
  mark_peg(peg_idx);
  s_game.ball.last_peg = peg_idx;

  const Peg *p = &s_game.board.pegs[peg_idx];
  int16_t bx = (int16_t)(s_game.ball.x >> FP_SHIFT);
  int16_t dx = (int16_t)(bx - p->x);

  int16_t chaos = settings_bounce_chaos();
  int16_t kick = (int16_t)(90 + (rand() % (chaos + 1)));
  if (dx == 0) {
    s_game.ball.vx = (rand() & 1) ? kick : (int16_t)(-kick);
  } else if (dx < 0) {
    s_game.ball.vx = (int32_t)(-kick - (rand() % 40));
  } else {
    s_game.ball.vx = (int32_t)(kick + (rand() % 40));
  }
  // Soften vertical so it keeps falling
  if (s_game.ball.vy > 120) s_game.ball.vy = 120;
  s_game.ball.vy = (s_game.ball.vy * 3) / 4;
  if (s_game.ball.vy < 30) s_game.ball.vy = 30;

  vibe_short();
}

static void physics_step(void) {
  Board *board = &s_game.board;
  Ball *ball = &s_game.ball;

  ball->vy += settings_gravity();
  if (ball->vy > 420) ball->vy = 420;

  // Cap horizontal speed
  if (ball->vx > 280) ball->vx = 280;
  if (ball->vx < -280) ball->vx = -280;

  ball->x += ball->vx;
  ball->y += ball->vy;

  int16_t bx = (int16_t)(ball->x >> FP_SHIFT);
  int16_t by = (int16_t)(ball->y >> FP_SHIFT);

  // Side walls
  const int16_t min_x = (int16_t)(board->left + BOARD_BALL_R);
  const int16_t max_x = (int16_t)(board->right - BOARD_BALL_R);
  if (bx < min_x) {
    bx = min_x;
    ball->x = ((int32_t)bx) << FP_SHIFT;
    ball->vx = -ball->vx / 2;
  } else if (bx > max_x) {
    bx = max_x;
    ball->x = ((int32_t)bx) << FP_SHIFT;
    ball->vx = -ball->vx / 2;
  }

  // Peg collisions (closest within hit radius)
  const int16_t hit_r = (int16_t)(BOARD_BALL_R + BOARD_PEG_R + 1);
  const int32_t hit_r2 = (int32_t)hit_r * hit_r;
  int8_t best = -1;
  int32_t best_d2 = hit_r2 + 1;
  for (uint8_t i = 0; i < board->peg_count; i++) {
    int16_t dx = (int16_t)(bx - board->pegs[i].x);
    int16_t dy = (int16_t)(by - board->pegs[i].y);
    int32_t d2 = (int32_t)dx * dx + (int32_t)dy * dy;
    if (d2 <= hit_r2 && d2 < best_d2) {
      best_d2 = d2;
      best = (int8_t)i;
    }
  }
  if (best >= 0) {
    resolve_peg_hit(best);
  }

  // Landed in bins
  if (by >= board->bottom) {
    int8_t bin = board_bin_at(board, bx);
    score_ball(bin);
  }
}

static void timer_cb(void *ctx) {
  (void)ctx;
  s_timer = NULL;
  game_tick();
  schedule_tick();
}

void game_init(void) {
  srand((unsigned)time(NULL));
  memset(&s_game, 0, sizeof(s_game));
  s_game.high_score = persist_exists(KEY_HIGH_SCORE)
                          ? (uint16_t)persist_read_int(KEY_HIGH_SCORE)
                          : 0;
  board_layout(&s_game.board, GSize(PBL_DISPLAY_WIDTH, PBL_DISPLAY_HEIGHT));
  s_game.aim_x = (int16_t)(PBL_DISPLAY_WIDTH / 2);
  clamp_aim();
  place_aim_ball();
  set_state(GameState_Title, "PLINKO");
  clear_recent_pegs();
  schedule_tick();
}

void game_deinit(void) {
  if (s_timer) {
    app_timer_cancel(s_timer);
    s_timer = NULL;
  }
}

void game_start_round(void) {
  s_game.score = 0;
  s_game.balls_total = settings_get()->balls_per_round;
  s_game.balls_left = s_game.balls_total;
  s_game.last_points = 0;
  s_game.last_bin = -1;
  s_game.aim_x = (int16_t)(PBL_DISPLAY_WIDTH / 2);
  clamp_aim();
  begin_aim();
}

void game_tick(void) {
  s_game.state_ms = (uint16_t)(s_game.state_ms + GAME_TICK_MS);

  switch (s_game.state) {
    case GameState_Falling:
      physics_step();
      break;
    case GameState_Scored:
      if (s_game.state_ms >= 700) {
        if (s_game.balls_left == 0) {
          save_high();
          set_state(GameState_RoundEnd, "ROUND OVER");
        } else {
          begin_aim();
        }
      }
      break;
    default:
      break;
  }
}

void game_select(void) {
  switch (s_game.state) {
    case GameState_Title:
      game_start_round();
      break;
    case GameState_Aim:
      drop_ball();
      break;
    case GameState_RoundEnd:
      game_start_round();
      break;
    default:
      break;
  }
}

void game_aim_up(void) {
  if (s_game.state == GameState_Title) {
    game_start_round();
    return;
  }
  if (s_game.state == GameState_RoundEnd) {
    set_state(GameState_Title, "PLINKO");
    return;
  }
  if (s_game.state != GameState_Aim) return;
  s_game.aim_x = (int16_t)(s_game.aim_x - settings_aim_step());
  clamp_aim();
  place_aim_ball();
}

void game_aim_down(void) {
  if (s_game.state == GameState_Title || s_game.state == GameState_RoundEnd) {
    game_start_round();
    return;
  }
  if (s_game.state != GameState_Aim) return;
  s_game.aim_x = (int16_t)(s_game.aim_x + settings_aim_step());
  clamp_aim();
  place_aim_ball();
}

const Game *game_get(void) {
  return &s_game;
}
