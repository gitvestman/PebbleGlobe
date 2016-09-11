#include <pebble.h>
#include "clock.h"
#include "message.h"
#include "globe.h"

static Layer *s_window_layer;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_time_shadow_layer;
static TextLayer *s_date_shadow_layer;
static GFont s_time_font;
static GFont s_date_font;
static long tick_count = 0;

#ifdef PBL_ROUND
static int timex = 25;
static int timey = 10;
static int datenx = -150;
static int dateny = -38;
#else
static int timex = 2;
static int timey = 0;
static int datenx = -100;
static int dateny = -38;
#endif

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
  tick_count = 0;
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
}

static void prv_unobstructed_did_change(void *context) {
  // Get the total available screen real-estate
  GRect bounds = layer_get_unobstructed_bounds(s_window_layer);
  int datex = bounds.size.w + datenx;
  int datey = bounds.size.h + dateny;

  APP_LOG(APP_LOG_LEVEL_DEBUG, "prv_unobstructed_did_change: w %d, h %d, x %d, y %d", bounds.size.w, bounds.size.h, datex, datey);

  // Move date textlayer
  layer_set_frame((Layer *)s_date_layer, GRect(datex,datey,100,45));

  // Move date shadow textlayer
  layer_set_frame((Layer *)s_date_shadow_layer, GRect(datex - 2, datey + 2,100,45));
  update_globe();
  spin_globe(0, 1);
}

void init_time(Window *window) {
  s_window_layer = window_get_root_layer(window);
  //Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  UnobstructedAreaHandlers handlers = {
    .did_change = prv_unobstructed_did_change
  };
  unobstructed_area_service_subscribe(handlers, NULL);

  GRect bounds = layer_get_bounds(s_window_layer);
  int datex = bounds.size.w + datenx;
  int datey = bounds.size.h + dateny;

  // Create time textlayer
  s_time_layer = text_layer_create(GRect(timex, timey, 130, 45));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, COLOR_FALLBACK(GColorPastelYellow , GColorWhite));

  // Create time shadow textlayer
  s_time_shadow_layer = text_layer_create(GRect(timex - 2,timey + 2,130,45));
  text_layer_set_background_color(s_time_shadow_layer, GColorClear);
  text_layer_set_text_color(s_time_shadow_layer, GColorBlack);

  // Create date textlayer
  s_date_layer = text_layer_create(GRect(datex,datey,100,45));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, COLOR_FALLBACK(GColorPastelYellow , GColorWhite));

  // Create date shadow textlayer
  s_date_shadow_layer = text_layer_create(GRect(datex - 2, datey + 2,100,45));
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
    strftime(timebuffer, sizeof(timebuffer), "%l:%M", tick_time);
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
  mktime(gm_time);
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