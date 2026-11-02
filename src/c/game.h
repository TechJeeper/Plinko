#pragma once

#include <pebble.h>
#include "board.h"

#define GAME_TICK_MS 33

typedef enum {
  GameState_Title = 0,
  GameState_Aim,
  GameState_Falling,
  GameState_Scored,
  GameState_RoundEnd,
} GameState;

typedef struct {
  int32_t x;  // fixed-point 8.8
  int32_t y;
  int32_t vx;
  int32_t vy;
  int8_t last_peg; // avoid re-colliding same peg every tick
} Ball;

typedef struct {
  GameState state;
  uint16_t state_ms;

  Board board;
  Ball ball;
  int16_t aim_x; // pixel drop column

  uint8_t balls_left;
  uint8_t balls_total;
  uint16_t score;
  uint16_t high_score;
  int16_t last_points;
  int8_t last_bin;

  char banner[28];
} Game;

void game_init(void);
void game_deinit(void);
void game_start_round(void);
void game_tick(void);
void game_select(void);
void game_aim_up(void);
void game_aim_down(void);
const Game *game_get(void);
