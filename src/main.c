#include <pebble.h>
#include "clock.h"
#include "math.h"
#include "globe.h"

Window *main_window;
//TextLayer *text_layer;
//static BitmapLayer *s_background_layer;

static void main_window_load(Window *window) {    
  // Make sure the time is displayed from the start
  init_globe(main_window);
  init_time(main_window);
  update_time();
}

static void main_window_unload(Window *window) {
  destroy_globe();
  destroy_time();
}

void handle_init(void) {
  main_window = window_create();

  window_set_window_handlers(main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  //text_layer = text_layer_create(GRect(0, 0, 144, 20));
  window_stack_push(main_window, true);
}

void handle_deinit(void) {
  //text_layer_destroy(text_layer);
  window_destroy(main_window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}

