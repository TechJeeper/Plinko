#include "settings.h"
#include "message_keys.auto.h"

#include <stdlib.h>

extern uint32_t MESSAGE_KEY_Vibrate;
extern uint32_t MESSAGE_KEY_Difficulty;
extern uint32_t MESSAGE_KEY_BallsPerRound;
extern uint32_t MESSAGE_KEY_ShowAimGuide;
extern uint32_t MESSAGE_KEY_BackgroundColor;
extern uint32_t MESSAGE_KEY_PegColor;
extern uint32_t MESSAGE_KEY_BallColor;

enum {
  PERSIST_VIBRATE = 10,
  PERSIST_DIFFICULTY = 11,
  PERSIST_BALLS = 12,
  PERSIST_SHOW_AIM = 13,
  PERSIST_BG_COLOR = 14,
  PERSIST_PEG_COLOR = 15,
  PERSIST_BALL_COLOR = 16,
};

static AppSettings s_settings;
static bool s_inbox_open;
static void (*s_changed)(void);

static int clamp_int(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static uint8_t normalize_balls(int v) {
  if (v <= 5) return 5;
  if (v <= 10) return 10;
  return 15;
}

static void settings_set_defaults(void) {
  s_settings.vibrate = true;
  s_settings.difficulty = DiffNormal;
  s_settings.balls_per_round = 10;
  s_settings.show_aim_guide = true;
#if defined(PBL_COLOR)
  s_settings.bg = GColorDukeBlue;
  s_settings.peg = GColorLightGray;
  s_settings.ball = GColorChromeYellow;
#else
  s_settings.bg = GColorBlack;
  s_settings.peg = GColorWhite;
  s_settings.ball = GColorWhite;
#endif
}

static void settings_force_mono(void) {
#if !defined(PBL_COLOR)
  s_settings.bg = GColorBlack;
  s_settings.peg = GColorWhite;
  s_settings.ball = GColorWhite;
#endif
}

static GColor color_from_persist(int key, GColor fallback) {
  if (!persist_exists(key)) return fallback;
  return GColorFromHEX(persist_read_int(key));
}

static void settings_load(void) {
  settings_set_defaults();
  if (persist_exists(PERSIST_VIBRATE)) {
    s_settings.vibrate = persist_read_bool(PERSIST_VIBRATE);
  }
  if (persist_exists(PERSIST_DIFFICULTY)) {
    s_settings.difficulty = (Difficulty)clamp_int(
        persist_read_int(PERSIST_DIFFICULTY), DiffEasy, DiffHard);
  }
  if (persist_exists(PERSIST_BALLS)) {
    s_settings.balls_per_round = normalize_balls(persist_read_int(PERSIST_BALLS));
  }
  if (persist_exists(PERSIST_SHOW_AIM)) {
    s_settings.show_aim_guide = persist_read_bool(PERSIST_SHOW_AIM);
  }
  s_settings.bg = color_from_persist(PERSIST_BG_COLOR, s_settings.bg);
  s_settings.peg = color_from_persist(PERSIST_PEG_COLOR, s_settings.peg);
  s_settings.ball = color_from_persist(PERSIST_BALL_COLOR, s_settings.ball);
  settings_force_mono();
}

static void settings_save(void) {
  persist_write_bool(PERSIST_VIBRATE, s_settings.vibrate);
  persist_write_int(PERSIST_DIFFICULTY, (int)s_settings.difficulty);
  persist_write_int(PERSIST_BALLS, (int)s_settings.balls_per_round);
  persist_write_bool(PERSIST_SHOW_AIM, s_settings.show_aim_guide);
  persist_write_int(PERSIST_BG_COLOR, (int)s_settings.bg.argb);
  persist_write_int(PERSIST_PEG_COLOR, (int)s_settings.peg.argb);
  persist_write_int(PERSIST_BALL_COLOR, (int)s_settings.ball.argb);
}

static int read_tuple_int(DictionaryIterator *iter, uint32_t key, int fallback) {
  Tuple *t = dict_find(iter, key);
  if (!t) return fallback;
  if (t->type == TUPLE_INT) return (int)t->value->int32;
  if (t->type == TUPLE_CSTRING) return atoi(t->value->cstring);
  return fallback;
}

static bool read_tuple_bool(DictionaryIterator *iter, uint32_t key, bool fallback) {
  Tuple *t = dict_find(iter, key);
  if (!t) return fallback;
  if (t->type == TUPLE_INT) return t->value->int32 != 0;
  if (t->type == TUPLE_CSTRING && t->length > 0) {
    const char *s = t->value->cstring;
    return s[0] == 't' || s[0] == 'T' || s[0] == '1';
  }
  return fallback;
}

static void inbox_received(DictionaryIterator *iter, void *context) {
  (void)context;
  s_settings.vibrate = read_tuple_bool(iter, MESSAGE_KEY_Vibrate, s_settings.vibrate);
  s_settings.difficulty = (Difficulty)clamp_int(
      read_tuple_int(iter, MESSAGE_KEY_Difficulty, (int)s_settings.difficulty),
      DiffEasy, DiffHard);
  s_settings.balls_per_round = normalize_balls(
      read_tuple_int(iter, MESSAGE_KEY_BallsPerRound, (int)s_settings.balls_per_round));
  s_settings.show_aim_guide =
      read_tuple_bool(iter, MESSAGE_KEY_ShowAimGuide, s_settings.show_aim_guide);

  Tuple *t = dict_find(iter, MESSAGE_KEY_BackgroundColor);
  if (t && t->type == TUPLE_INT) {
    s_settings.bg = GColorFromHEX(t->value->int32);
  }
  t = dict_find(iter, MESSAGE_KEY_PegColor);
  if (t && t->type == TUPLE_INT) {
    s_settings.peg = GColorFromHEX(t->value->int32);
  }
  t = dict_find(iter, MESSAGE_KEY_BallColor);
  if (t && t->type == TUPLE_INT) {
    s_settings.ball = GColorFromHEX(t->value->int32);
  }

  settings_force_mono();
  settings_save();
  if (s_changed) s_changed();
}

const AppSettings *settings_get(void) {
  return &s_settings;
}

int16_t settings_gravity(void) {
  switch (s_settings.difficulty) {
    case DiffEasy: return 18;   // ~0.07 px/tick^2 in 8.8 fixed
    case DiffHard: return 36;
    case DiffNormal:
    default: return 26;
  }
}

int16_t settings_bounce_chaos(void) {
  switch (s_settings.difficulty) {
    case DiffEasy: return 25;
    case DiffHard: return 70;
    case DiffNormal:
    default: return 45;
  }
}

int16_t settings_aim_step(void) {
  switch (s_settings.difficulty) {
    case DiffEasy: return 8;
    case DiffHard: return 4;
    case DiffNormal:
    default: return 6;
  }
}

void settings_on_changed(void (*cb)(void)) {
  s_changed = cb;
}

void settings_init(void) {
  settings_load();
  if (!s_inbox_open) {
    app_message_register_inbox_received(inbox_received);
    app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
    s_inbox_open = true;
  }
}

void settings_deinit(void) {
  if (s_inbox_open) {
    app_message_deregister_callbacks();
    s_inbox_open = false;
  }
}
