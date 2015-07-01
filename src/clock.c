#include <pebble.h>
#include "clock.h"
#include "message.h"
#include "globe.h"

static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_time_shadow_layer;
static TextLayer *s_date_shadow_layer;
static GFont s_time_font;
static GFont s_date_font;
static long tick_count = 0;

// Change to minute ticking after a while to save battery
#define MAX_SECOND_TICKS 20

void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  tick_count++;
  if (tick_count == MAX_SECOND_TICKS && units_changed == SECOND_UNIT) {
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  }
}

void reset_ticks() {
  BatteryChargeState charge = battery_state_service_peek();
  if (charge.is_charging || charge.charge_percent > 20) {
    tick_count = 0;
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);    
  }
}

void init_time(Window *window) {
  //Register with TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  
  // Create time textlayer
  s_time_layer = text_layer_create(GRect(2,0,130,45));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, COLOR_FALLBACK(GColorPastelYellow , GColorWhite));

  // Create time shadow textlayer
  s_time_shadow_layer = text_layer_create(GRect(0,2,130,45));
  text_layer_set_background_color(s_time_shadow_layer, GColorClear);
  text_layer_set_text_color(s_time_shadow_layer, GColorBlack);

  // Create date textlayer
  s_date_layer = text_layer_create(GRect(40,130,100,45));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, COLOR_FALLBACK(GColorPastelYellow , GColorWhite));

  // Create date shadow textlayer
  s_date_shadow_layer = text_layer_create(GRect(38,132,100,45));
  text_layer_set_background_color(s_date_shadow_layer, GColorClear);
  text_layer_set_text_color(s_date_shadow_layer, GColorBlack);

  // Create Fonts
  s_time_font = fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT);
  s_date_font = fonts_get_system_font(FONT_KEY_GOTHIC_28);
  
  text_layer_set_font(s_time_shadow_layer, s_time_font);
  text_layer_set_font(s_date_shadow_layer, s_date_font);
  text_layer_set_text_alignment(s_time_shadow_layer, GTextAlignmentLeft);
  text_layer_set_text_alignment(s_date_shadow_layer, GTextAlignmentRight);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_shadow_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_shadow_layer));

  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_font(s_date_layer, s_date_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentLeft);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentRight);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
}

void update_time() {
  // Get a tm structure
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);
  
  // Create longed-lived buffers
  static char timebuffer[] = "00:00 ";
  static char datebuffer[] = "Tuesday 31 ";                              
  static char timeshadowbuffer[] = "00:00 ";
  static char dateshadowbuffer[] = "Tuesday 31 ";                              
  
  // Write the current hours and minutes into the buffer
  if (clock_is_24h_style()) {
    // use 24 hour format  
    strftime(timebuffer, sizeof(timebuffer), "%H:%M", tick_time);
  } else {
    strftime(timebuffer, sizeof(timebuffer), "%I:%M", tick_time);
  }
  strncpy(timeshadowbuffer, timebuffer, sizeof(timebuffer));
  strftime(datebuffer, sizeof(datebuffer), "%a %e", tick_time);
  strncpy(dateshadowbuffer, datebuffer, sizeof(timebuffer));

  // Display the time and date
  text_layer_set_text(s_time_layer, timebuffer);
  text_layer_set_text(s_date_layer, datebuffer);  
  text_layer_set_text(s_time_shadow_layer, timeshadowbuffer);
  text_layer_set_text(s_date_shadow_layer, dateshadowbuffer);  
  
  // Calculate sun position
  struct tm *gm_time = gmtime(&now);  
#if PBL_PLATFORM_APLITE
  gm_time->tm_hour += timezone_offset / 60;
#endif
  int16_t sunlong = (int16_t)(TRIG_MAX_ANGLE/2 + TRIG_MAX_ANGLE/4 - (int)(gm_time->tm_hour * 2730.6 + gm_time->tm_min * 45.5));
  int16_t sunlat = (int16_t)((cos_lookup((gm_time->tm_yday - 7) * TRIG_MAX_ANGLE / 365))/32) - 0x200;
  set_sun_position(sunlong, sunlat);
}

void destroy_time() {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_time_shadow_layer);
  text_layer_destroy(s_date_shadow_layer);
  tick_timer_service_unsubscribe();
}
