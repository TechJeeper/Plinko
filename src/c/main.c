#include <pebble.h>
#include "game.h"
#include "settings.h"
#include "ui.h"

static void on_settings_changed(void) {
  ui_apply_settings();
}

static void init(void) {
  settings_init();
  settings_on_changed(on_settings_changed);
  game_init();
  ui_init();
}

static void deinit(void) {
  ui_deinit();
  game_deinit();
  settings_deinit();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
