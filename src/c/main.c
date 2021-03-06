#include <pebble.h>
#include "clock.h"
//#include "math.h"
#include "health.h"
#include "globe.h"
#include "battery.h"
#include "message.h"

Window *main_window;

static void tap_handler(AccelAxisType axis, int32_t direction) {
  static time_t last_tap = 0;
  time_t seconds;

  time_ms(&seconds, NULL);

  if (seconds - last_tap < 5) {
    last_tap = seconds;
    return;
  }

  last_tap = seconds;
  if (app_config.animations)
    spin_globe(0, direction);
}

static void main_unobstructed_did_change(void *context) {
    clock_unobstructed_did_change(context);
    #ifdef PBL_HEALTH
    health_unobstructed_did_change(context);
    #endif
}

static void main_window_load(Window *window) {
  init_globe(main_window);
  #ifdef PBL_HEALTH
  init_health(main_window);
  #endif
  init_time(main_window);
  init_battery(main_window);
  update_time();
}

static void main_window_unload(Window *window) {
  destroy_globe();
  #ifdef PBL_HEALTH
  destroy_health();
  #endif
  destroy_time();
  destroy_battery();
}

void handle_init(void) {
  main_window = window_create();

  window_set_window_handlers(main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  window_stack_push(main_window, true);

  // Subscribe to taps
  accel_tap_service_subscribe(tap_handler);

  // Subscribe to Quick View events
  UnobstructedAreaHandlers handlers = {
    .did_change = main_unobstructed_did_change
  };
  unobstructed_area_service_subscribe(handlers, NULL);

  // Register callbacks
  message_init(main_window);
}

void handle_deinit(void) {
  accel_tap_service_unsubscribe();
  unobstructed_area_service_unsubscribe();
  message_deinit();
  window_destroy(main_window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
