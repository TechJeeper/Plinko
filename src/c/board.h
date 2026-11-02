#pragma once
#include <pebble.h>

#define BOARD_MAX_PEGS 96
#define BOARD_MAX_BINS 7
#define BOARD_BALL_R   4
#define BOARD_PEG_R    2

typedef struct {
  int16_t x;
  int16_t y;
} Peg;

typedef struct {
  int16_t x0;       // left edge of bin slot
  int16_t x1;       // right edge
  int16_t multiplier;
} Bin;

typedef struct {
  int16_t top;          // y of first peg row
  int16_t bottom;       // y where bins start
  int16_t left;         // playfield left
  int16_t right;        // playfield right
  int16_t drop_y;       // resting y while aiming
  int16_t bin_label_y;  // text baseline for multipliers
  Peg pegs[BOARD_MAX_PEGS];
  uint8_t peg_count;
  Bin bins[BOARD_MAX_BINS];
  uint8_t bin_count;
} Board;

void board_layout(Board *board, GSize display);
int8_t board_bin_at(const Board *board, int16_t x);
