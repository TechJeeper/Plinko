#include "ui.h"
#include "game.h"
#include "settings.h"

#include <stdio.h>

static Window *s_window;
static Layer *s_canvas;
static AppTimer *s_redraw = NULL;
static GBitmap *s_bmp_ball = NULL;
static GBitmap *s_bmp_peg = NULL;
static GBitmap *s_bmp_title = NULL;

#if defined(PBL_COLOR)
#define C_TEXT   GColorWhite
#define C_DIM    GColorLightGray
#define C_WALL   GColorDarkGray
#define C_BIN_HI GColorYellow
#else
#define C_TEXT   GColorWhite
#define C_DIM    GColorWhite
#define C_WALL   GColorWhite
#define C_BIN_HI GColorWhite
#endif

static void redraw_cb(void *ctx) {
  (void)ctx;
  s_redraw = NULL;
  if (s_canvas) layer_mark_dirty(s_canvas);
  s_redraw = app_timer_register(50, redraw_cb, NULL);
}

static void draw_bitmap_centered(GContext *ctx, GBitmap *bmp, GPoint center) {
  if (!bmp) return;
  GRect b = gbitmap_get_bounds(bmp);
  GRect dest = GRect(center.x - b.size.w / 2, center.y - b.size.h / 2, b.size.w, b.size.h);
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, bmp, dest);
}

static void draw_hud(GContext *ctx, GRect bounds, const Game *g) {
  char line[32];
  graphics_context_set_text_color(ctx, C_TEXT);

  snprintf(line, sizeof(line), "S:%u  H:%u", (unsigned)g->score, (unsigned)g->high_score);
  graphics_draw_text(ctx, line, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(2, 0, bounds.size.w - 4, 16),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  if (g->state != GameState_Title) {
    snprintf(line, sizeof(line), "B:%u", (unsigned)g->balls_left);
    graphics_draw_text(ctx, line, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                       GRect(2, 0, bounds.size.w - 4, 16),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
  }
}

static void draw_board(GContext *ctx, const Game *g) {
  const Board *b = &g->board;
  const AppSettings *s = settings_get();

  // Side rails
  graphics_context_set_stroke_color(ctx, C_WALL);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, GPoint(b->left - 1, b->drop_y), GPoint(b->left - 1, b->bottom));
  graphics_draw_line(ctx, GPoint(b->right + 1, b->drop_y), GPoint(b->right + 1, b->bottom));

  // Pegs
  for (uint8_t i = 0; i < b->peg_count; i++) {
    GPoint p = GPoint(b->pegs[i].x, b->pegs[i].y);
    if (s_bmp_peg) {
      draw_bitmap_centered(ctx, s_bmp_peg, p);
    } else {
      graphics_context_set_fill_color(ctx, s->peg);
      graphics_fill_circle(ctx, p, BOARD_PEG_R);
    }
  }

  // Bin dividers + labels
  graphics_context_set_stroke_color(ctx, C_WALL);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, GPoint(b->left, b->bottom), GPoint(b->right, b->bottom));

  for (uint8_t i = 0; i < b->bin_count; i++) {
    const Bin *bin = &b->bins[i];
    graphics_draw_line(ctx, GPoint(bin->x0, b->bottom), GPoint(bin->x0, b->bottom + 18));

    bool highlight = (g->last_bin == (int8_t)i) &&
                     (g->state == GameState_Scored || g->state == GameState_RoundEnd);
    graphics_context_set_text_color(ctx, highlight ? C_BIN_HI : C_DIM);

    char label[8];
    snprintf(label, sizeof(label), "%d", (int)bin->multiplier);
    int16_t mid = (int16_t)((bin->x0 + bin->x1) / 2);
    graphics_draw_text(ctx, label, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(mid - 10, b->bin_label_y, 20, 16),
                       GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  }
  graphics_draw_line(ctx, GPoint(b->right, b->bottom), GPoint(b->right, b->bottom + 18));
}

static void draw_ball(GContext *ctx, const Game *g) {
  int16_t bx = (int16_t)(g->ball.x >> 8);
  int16_t by = (int16_t)(g->ball.y >> 8);

  if (s_bmp_ball) {
    draw_bitmap_centered(ctx, s_bmp_ball, GPoint(bx, by));
  } else {
    graphics_context_set_fill_color(ctx, settings_get()->ball);
    graphics_fill_circle(ctx, GPoint(bx, by), BOARD_BALL_R);
  }
}

static void draw_aim_guide(GContext *ctx, const Game *g) {
  if (!settings_get()->show_aim_guide) return;
  if (g->state != GameState_Aim) return;

  graphics_context_set_stroke_color(ctx, C_DIM);
  graphics_context_set_stroke_width(ctx, 1);
  // Dashed vertical guide
  for (int16_t y = g->board.drop_y + 6; y < g->board.top; y = (int16_t)(y + 4)) {
    graphics_draw_line(ctx, GPoint(g->aim_x, y), GPoint(g->aim_x, (int16_t)(y + 2)));
  }
}

static void draw_overlays(GContext *ctx, GRect bounds, const Game *g) {
  graphics_context_set_text_color(ctx, C_TEXT);

  if (g->state == GameState_Title) {
    if (s_bmp_title) {
      GRect tb = gbitmap_get_bounds(s_bmp_title);
      GRect dest = GRect((bounds.size.w - tb.size.w) / 2, 36, tb.size.w, tb.size.h);
      graphics_context_set_compositing_mode(ctx, GCompOpSet);
      graphics_draw_bitmap_in_rect(ctx, s_bmp_title, dest);
    } else {
      graphics_draw_text(ctx, "PLINKO", fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD),
                         GRect(2, 50, bounds.size.w - 4, 32),
                         GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    }
    graphics_context_set_text_color(ctx, C_DIM);
    graphics_draw_text(ctx, "SELECT to play", fonts_get_system_font(FONT_KEY_GOTHIC_18),
                       GRect(2, bounds.size.h - 40, bounds.size.w - 4, 20),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    char hi[24];
    snprintf(hi, sizeof(hi), "Best %u", (unsigned)g->high_score);
    graphics_draw_text(ctx, hi, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(2, bounds.size.h - 22, bounds.size.w - 4, 18),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    return;
  }

  if (g->banner[0] && g->state != GameState_Falling) {
    graphics_context_set_text_color(ctx, C_TEXT);
    graphics_draw_text(ctx, g->banner, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                       GRect(2, 14, bounds.size.w - 4, 20),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  }

  if (g->state == GameState_RoundEnd) {
    char line[32];
    snprintf(line, sizeof(line), "Score %u", (unsigned)g->score);
    graphics_draw_text(ctx, line, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                       GRect(2, bounds.size.h / 2 - 20, bounds.size.w - 4, 28),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    graphics_context_set_text_color(ctx, C_DIM);
    graphics_draw_text(ctx, "SELECT: again  UP: menu",
                       fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(2, bounds.size.h - 22, bounds.size.w - 4, 18),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  }
}

static void canvas_update(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  const Game *g = game_get();

  graphics_context_set_fill_color(ctx, settings_get()->bg);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  if (g->state != GameState_Title) {
    draw_hud(ctx, bounds, g);
    draw_board(ctx, g);
    draw_aim_guide(ctx, g);
    if (g->state == GameState_Aim || g->state == GameState_Falling ||
        g->state == GameState_Scored) {
      draw_ball(ctx, g);
    }
  }

  draw_overlays(ctx, bounds, g);
}

static void select_click(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer;
  (void)context;
  game_select();
  ui_mark_dirty();
}

static void up_click(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer;
  (void)context;
  game_aim_up();
  ui_mark_dirty();
}

static void down_click(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer;
  (void)context;
  game_aim_down();
  ui_mark_dirty();
}

static void click_config(void *context) {
  (void)context;
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click);
  window_single_click_subscribe(BUTTON_ID_UP, up_click);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click);
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 80, up_click);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 80, down_click);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_canvas = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_canvas, canvas_update);
  layer_add_child(root, s_canvas);

  s_bmp_ball = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BALL);
  s_bmp_peg = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PEG);
  s_bmp_title = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TITLE);

  s_redraw = app_timer_register(50, redraw_cb, NULL);
}

static void window_unload(Window *window) {
  (void)window;
  if (s_redraw) {
    app_timer_cancel(s_redraw);
    s_redraw = NULL;
  }
  if (s_bmp_ball) { gbitmap_destroy(s_bmp_ball); s_bmp_ball = NULL; }
  if (s_bmp_peg) { gbitmap_destroy(s_bmp_peg); s_bmp_peg = NULL; }
  if (s_bmp_title) { gbitmap_destroy(s_bmp_title); s_bmp_title = NULL; }
  layer_destroy(s_canvas);
  s_canvas = NULL;
}

void ui_init(void) {
  s_window = window_create();
  window_set_background_color(s_window, settings_get()->bg);
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_set_click_config_provider(s_window, click_config);
  window_stack_push(s_window, true);
}

void ui_deinit(void) {
  window_destroy(s_window);
  s_window = NULL;
}

void ui_mark_dirty(void) {
  if (s_canvas) layer_mark_dirty(s_canvas);
}

void ui_apply_settings(void) {
  if (s_window) {
    window_set_background_color(s_window, settings_get()->bg);
  }
  ui_mark_dirty();
}
