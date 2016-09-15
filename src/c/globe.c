#include <pebble.h>
#include "clock.h"
#include "globe.h"
#include "ball.h"
#include "message.h"

#define FIXED_360_DEG 0x10000
#define FIXED_360_DEG_SHIFT 16

static GBitmap *s_globe_bitmap;
static Layer *s_simple_bg_layer;
static Layer *window_layer;

static Ball globe;

static int8_t globeradius = 60;
static uint_fast8_t globecenterx, globecentery;
static uint16_t globelong = 90;
static int globelat = 0xF000;
static int xres = 0;
static int yres = 0;
static int sunlong = 0;
static int sunlat = 0;
static int animation_direction = 1;
static bool animating = true;
static int animation_count;
static bool gpsposition = 0;

static void update_animaion_parameters();

void set_sun_position(uint16_t longitude, int16_t latitude) {
  sunlong = longitude;
  sunlat = latitude;
  //APP_LOG(APP_LOG_LEVEL_INFO, "set_sun_position: sunlong %d, sunlat %d", sunlong, sunlat);
  if (animating) {
    update_animaion_parameters();
  } else {
    layer_mark_dirty(s_simple_bg_layer);
  }
  gpsposition = true;
}

static void bg_update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, app_config.inverted ? GColorWhite : GColorBlack);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);

  if (!animating) {
    globelong = sunlong;
    globelat = sunlat;
  }

  ball_update_proc(globe, layer, ctx, globelat, globelong);

  if (currentlong != 0 && gpsposition) {
    draw_gps_position(globe, layer, ctx, globelat, globelong, currentlong, currentlat);
  }
}

static int longitude_start = 0;
static int longitude_length = 0;
static int latitude_start = 0;
static int latitude_length = 0;
static Animation* s_globe_animation;
#define ANIMATION_DURATION 5000
#define ANIMATION_INITIAL_DELAY 500

static void anim_started_handler(Animation* anim, void* context) {
  longitude_start = globelong;
  longitude_length = abs(sunlong - globelong) + FIXED_360_DEG;
  latitude_start = globelat;
  latitude_length = sunlat - globelat;
  animating = true;
  animation_count = 0;
  tick_timer_service_unsubscribe();
}

static void update_animaion_parameters() {
  longitude_length = abs(sunlong - longitude_start) + FIXED_360_DEG;
}

static void anim_stopped_handler(Animation* anim, bool finished, void* context) {
  animation_destroy(anim);
  animating = false;
  reset_ticks();
  //APP_LOG(APP_LOG_LEVEL_INFO, "Animation count %d", animation_count);
}

static void anim_update_handler(Animation* anim, AnimationProgress progress) {
  globelong = longitude_start + animation_direction * longitude_length * progress / ANIMATION_NORMALIZED_MAX;
  globelat = latitude_start + latitude_length * progress / ANIMATION_NORMALIZED_MAX;
  layer_mark_dirty(s_simple_bg_layer);
  animation_count++;
}

static AnimationImplementation spin_animation = {
   .update = anim_update_handler
};

void spin_globe(int delay, int direction) {
  s_globe_animation = animation_create();
  animation_set_delay((Animation*)s_globe_animation, delay);
  animation_set_duration((Animation*)s_globe_animation, ANIMATION_DURATION);
  animation_set_curve((Animation*)s_globe_animation, delay != 0 ? AnimationCurveEaseInOut : AnimationCurveEaseOut);
  animation_set_handlers((Animation*)s_globe_animation, (AnimationHandlers) {
    .started = anim_started_handler,
    .stopped = anim_stopped_handler
  }, NULL);
  animation_direction = direction;
  animation_set_implementation((Animation*)s_globe_animation, &spin_animation);
  animation_schedule((Animation*)s_globe_animation);
}

void init_globe(Window *window) {
  init_sqrt();

  window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_unobstructed_bounds(window_layer);
  globecenterx = bounds.size.w / 2;
  globecentery = bounds.size.h / 2;
  if (globecentery > 40) globecentery += 10;
  xres = bounds.size.w;
  yres = bounds.size.h;
  // Create GBitmap, then set to created BitmapLayer
  s_globe_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_GLOBE);
  //int bytes = gbitmap_get_bytes_per_row(s_globe_bitmap);
  //APP_LOG(APP_LOG_LEVEL_INFO, "Bytes per row: %d", bytes);
  //GBitmapFormat format = gbitmap_get_format(s_globe_bitmap);
  //APP_LOG(APP_LOG_LEVEL_INFO, "Bitmap format: %d", (int)format);
  globe = create_ball(s_globe_bitmap, globeradius, globecenterx, globecentery);

  s_simple_bg_layer = layer_create(bounds);
  layer_set_update_proc(s_simple_bg_layer, bg_update_proc);
  layer_add_child(window_get_root_layer(window), s_simple_bg_layer);
  spin_globe(ANIMATION_INITIAL_DELAY, 1);
}

void update_globe() {
  GRect bounds = layer_get_unobstructed_bounds(window_layer);
  layer_set_bounds(s_simple_bg_layer, bounds);
  globecenterx = bounds.size.w / 2;
  globecentery = bounds.size.h / 2;
  if (globecentery > 40) globecentery += 10;
  xres = bounds.size.w;
  yres = bounds.size.h;

  update_ball(globe, globeradius, globecenterx, globecentery);
  layer_mark_dirty(s_simple_bg_layer);
}

void destroy_globe() {
  destroy_ball(globe);
  gbitmap_destroy(s_globe_bitmap);
  layer_destroy(s_simple_bg_layer);
}
