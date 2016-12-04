#include <pebble.h>
#include "clock.h"
#include "message.h"
#include "globe.h"

extern uint_fast8_t globecenterx, globecentery;

static Layer *s_window_layer;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_time_shadow_layer;
static TextLayer *s_date_shadow_layer;
static GFont s_time_font;
static GFont s_time_bold_font;
static GFont s_date_font;
static GFont s_date_bold_font;
static long tick_count = 0;

#ifdef PBL_ROUND
static int defaulttimex = 25;
static int defaulttimey = 10;
static int timex = 25;
static int timey = 10;
static int datenx = -170;
static int dateny = -38;
#else
static int defaulttimex = 2;
static int defaulttimey = 0;
static int timex = 2;
static int timey = 0;
static int datenx = -120;
static int dateny = -30;
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

void clock_unobstructed_did_change(void *context) {
  // Get the total available screen real-estate
  GRect bounds = layer_get_unobstructed_bounds(s_window_layer);
  int datex = bounds.size.w + datenx;
  int datey = bounds.size.h + dateny;

  // Move date textlayer
  layer_set_frame((Layer *)s_date_layer, GRect(datex,datey,118,30));

  // Move date shadow textlayer
  layer_set_frame((Layer *)s_date_shadow_layer, GRect(datex - 2, datey + 2,118,30));
  update_globe();
  if (app_config.animations)
    spin_globe(0, 1);
}

static void set_colors() {
  GColor background = GColorClear;
  GColor shadowcolor = app_config.inverted || app_config.center ? GColorWhite : GColorBlack;
  GColor textcolor = app_config.inverted || app_config.center ? COLOR_FALLBACK(GColorDukeBlue , GColorBlack) : COLOR_FALLBACK(GColorPastelYellow , GColorWhite);

  text_layer_set_background_color(s_time_layer, background);
  text_layer_set_text_color(s_time_layer, textcolor);

  text_layer_set_background_color(s_time_shadow_layer, background);
  text_layer_set_text_color(s_time_shadow_layer, shadowcolor);

  text_layer_set_background_color(s_date_layer, background);
  text_layer_set_text_color(s_date_layer, textcolor);

  text_layer_set_background_color(s_date_shadow_layer, background);
  text_layer_set_text_color(s_date_shadow_layer, shadowcolor);
}

static void set_fonts() {
  text_layer_set_font(s_time_shadow_layer, app_config.bold ? s_time_bold_font : s_time_font);

  text_layer_set_font(s_time_layer, app_config.bold ? s_time_bold_font : s_time_font);

  text_layer_set_font(s_date_shadow_layer, app_config.bold ? s_date_bold_font : s_date_font);

  text_layer_set_font(s_date_layer, app_config.bold ? s_date_bold_font : s_date_font);
}

void init_time(Window *window) {
  s_window_layer = window_get_root_layer(window);
  //Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  GRect bounds = layer_get_unobstructed_bounds(s_window_layer);
  int datex = bounds.size.w + datenx;
  int datey = bounds.size.h + dateny;

  if (app_config.center)
  {
    timex = globecenterx - 65;
    timey = globecentery - 24;
  } else {
    timex = defaulttimex;
    timey = defaulttimey;
  }

  // Create time textlayer
  s_time_layer = text_layer_create(GRect(timex, timey, 130, 45));

  // Create time shadow textlayer
  s_time_shadow_layer = text_layer_create(GRect(timex - 2,timey - 2,130,45));

  // Create date textlayer
  s_date_layer = text_layer_create(GRect(datex,datey,118,45));

  // Create date shadow textlayer
  s_date_shadow_layer = text_layer_create(GRect(datex - 2, datey + 2,118,45));

  // Create Fonts
  s_time_font = fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT);
  s_time_bold_font = fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS);
  s_date_font = fonts_get_system_font(FONT_KEY_GOTHIC_28);
  s_date_bold_font = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);

  text_layer_set_text_alignment(s_time_shadow_layer, GTextAlignmentLeft);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_shadow_layer));

  text_layer_set_text_alignment(s_time_layer, GTextAlignmentLeft);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));

  text_layer_set_text_alignment(s_date_shadow_layer, GTextAlignmentRight);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_shadow_layer));

  text_layer_set_text_alignment(s_date_layer, GTextAlignmentRight);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));

  if (!app_config.showDate) {
    layer_set_hidden((Layer *)s_date_layer, true);
    layer_set_hidden((Layer *)s_date_shadow_layer, true);
  }

  set_colors();
  set_fonts();
}

void update_time() {
  set_colors();
  set_fonts();
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
  // Display the time
  text_layer_set_text(s_time_layer, timebuffer);
  text_layer_set_text(s_time_shadow_layer, timeshadowbuffer);

  if (app_config.center)
  {
    timex = globecenterx - 65;
    timey = globecentery - 25;
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
    text_layer_set_text_alignment(s_time_shadow_layer, GTextAlignmentCenter);    
  }
  else
  {
    timex = defaulttimex;
    timey = defaulttimey;
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentLeft);
    text_layer_set_text_alignment(s_time_shadow_layer, GTextAlignmentLeft);    
  }
  // Move time textlayers
  layer_set_frame((Layer *)s_time_layer, GRect(timex, timey, 130, 45));
  layer_set_frame((Layer *)s_time_shadow_layer, GRect(timex - 2,timey - 2,130,45));

  if (app_config.showDate) {
    layer_set_hidden((Layer *)s_date_layer, false);
    layer_set_hidden((Layer *)s_date_shadow_layer, false);

    strftime(datebuffer, sizeof(datebuffer), "%a %e", tick_time);
    strncpy(dateshadowbuffer, datebuffer, sizeof(datebuffer));

    text_layer_set_text(s_date_layer, datebuffer);
    text_layer_set_text(s_date_shadow_layer, dateshadowbuffer);
  } else {
    layer_set_hidden((Layer *)s_date_layer, true);
    layer_set_hidden((Layer *)s_date_shadow_layer, true);
  }
}

void destroy_time() {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_time_shadow_layer);
  text_layer_destroy(s_date_shadow_layer);
  tick_timer_service_unsubscribe();
}
