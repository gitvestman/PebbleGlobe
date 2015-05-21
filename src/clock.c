#include <pebble.h>
#include "clock.h"

static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static GFont s_time_font;
static GFont s_date_font;

void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

void init_time(Window *window) {
  //Register with TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  
  // Create time textlayer
  s_time_layer = text_layer_create(GRect(2,2,120,45));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, COLOR_FALLBACK(GColorPastelYellow , GColorWhite));

  // Create time textlayer
  s_date_layer = text_layer_create(GRect(40,130,100,45));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, COLOR_FALLBACK(GColorPastelYellow , GColorWhite));

  // Create Fonts
  s_time_font = fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT);
  s_date_font = fonts_get_system_font(FONT_KEY_GOTHIC_28);
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_font(s_date_layer, s_date_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentLeft);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentRight);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
}

void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  // Create longed-lived buffers
  static char timebuffer[] = "00:00";
  static char datebuffer[] = "Tuesday 31";                              
  
  // Write the current hours and minutes into the buffer
  if (clock_is_24h_style()) {
    // use 24 hour format  
    strftime(timebuffer, sizeof(timebuffer), "%H:%M", tick_time);
  } else {
    strftime(timebuffer, sizeof(timebuffer), "%I, %M", tick_time);
  }  
  strftime(datebuffer, sizeof(datebuffer), "%a %e", tick_time);

  // Display the time and date
  text_layer_set_text(s_time_layer, timebuffer);
  text_layer_set_text(s_date_layer, datebuffer);  
}

void destroy_time() {
  text_layer_destroy(s_time_layer);
  tick_timer_service_unsubscribe();
}
