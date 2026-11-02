#pragma once
#include <pebble.h>

typedef enum {
  DiffEasy = 0,
  DiffNormal = 1,
  DiffHard = 2,
} Difficulty;

typedef struct {
  bool vibrate;
  Difficulty difficulty;
  uint8_t balls_per_round; // 5, 10, or 15
  bool show_aim_guide;
  GColor bg;
  GColor peg;
  GColor ball;
} AppSettings;

void settings_init(void);
void settings_deinit(void);
const AppSettings *settings_get(void);

// Gravity (fixed-point velocity add per tick).
int16_t settings_gravity(void);
// Peg bounce randomness 0..100 (higher = wilder).
int16_t settings_bounce_chaos(void);
// Aim column step in pixels.
int16_t settings_aim_step(void);

void settings_on_changed(void (*cb)(void));
