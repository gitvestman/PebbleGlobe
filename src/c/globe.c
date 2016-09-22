#include <pebble.h>
#include "clock.h"
#include "globe.h"
#include "ball.h"

static GBitmap *s_body_bitmap;
static GBitmap *s_head_bitmap;
static Layer *s_simple_bg_layer;
static Layer *window_layer;

static Ball body;
static Ball head;

static int8_t maxgloberadius = 48;
static int16_t globeradius = 48;
static uint_fast8_t globecenterx, globecentery;

static int16_t headradius = 28;
static uint_fast8_t headcenterx, headcentery;

static uint16_t globelong = 15000;
static int globelat = 0x2000;

static uint16_t headlong = 15000;
static int headlat = -0x1000;
static int headbump = 0;

static int xres = 0;
static int yres = 0;
static int sunlong = 0xA000;
static int animation_direction = 1;
static int animation_count;
bool animating = true;
bool firstframe = true;

#define FIXED_360_DEG_SHIFT 16
#define FIXED_360_DEG 0x10000



static void bg_update_proc(Layer *layer, GContext *ctx) {
  if (!animating) {
    globelat -= 128 * animation_direction;
    globelong += 64 * animation_direction;
    headlong += 256 * animation_direction;
    if (headlong < 100 || headlong > 20000)
      animation_direction = -animation_direction;
  }

  ball_update_proc(body, layer, ctx, globelat, globelong, 0, 0);
  ball_update_proc(head, layer, ctx, headlat, headlong, 0, headbump);
}

static int longitude_start = 0;
static int longitude_length = 0;
static int latitude_start = 0;
static int latitude_length = 0;
static int headlong_start = 0;
static Animation* s_globe_animation;
#define ANIMATION_DURATION 5000
#define ANIMATION_INITIAL_DELAY 500

static void anim_started_handler(Animation* anim, void* context) {
  longitude_start = globelong;
  longitude_length = abs(sunlong - globelong) + FIXED_360_DEG * 2;
  latitude_start = globelat;
  latitude_length = abs(sunlong - globelong) + FIXED_360_DEG;
  headlong_start = headlong;
  animating = true;
  firstframe = true;
  animation_count = 0;
  tick_timer_service_unsubscribe();
}

static void anim_stopped_handler(Animation* anim, bool finished, void* context) {
  animation_destroy(anim);
  animating = false;
  reset_ticks();
  //APP_LOG(APP_LOG_LEVEL_INFO, "Animation count %d", animation_count);
}

static void anim_update_handler(Animation* anim, AnimationProgress progress) {
  globelong = longitude_start + animation_direction * longitude_length * progress / ANIMATION_NORMALIZED_MAX;
  globelat = latitude_start + animation_direction * latitude_length * progress / ANIMATION_NORMALIZED_MAX;
  headlong = headlong_start + animation_direction * sin_lookup(progress * 3) / 8;
  headbump = abs(3 * cos_lookup(progress * 8) >> FIXED_360_DEG_SHIFT);
  layer_mark_dirty(s_simple_bg_layer);
  if (animation_count > 1) firstframe = false;
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
  globecenterx = headcenterx = bounds.size.w / 2;
  globecentery = bounds.size.h / 2 + 20;
  headcentery = globecentery - globeradius + 0;
  xres = bounds.size.w;
  yres = bounds.size.h;

  // Body
  s_body_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_GLOBE);
  //int bytes = gbitmap_get_bytes_per_row(s_body_bitmap);
  //GRect bodysize = gbitmap_get_bounds(s_body_bitmap);
  //APP_LOG(APP_LOG_LEVEL_INFO, "Globe Bytes per row: %d (%d, %d)", bytes, bodysize.size.w, bodysize.size.h);
  //GBitmapFormat format = gbitmap_get_format(s_body_bitmap);
  //APP_LOG(APP_LOG_LEVEL_INFO, "Globe Bitmap format: %d", (int)format);
  body = create_ball(s_body_bitmap, globeradius, globecenterx, globecentery);
  //APP_LOG(APP_LOG_LEVEL_INFO, "Body created: %p", body);

  // Head
  s_head_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HEAD);
  //bytes = gbitmap_get_bytes_per_row(s_head_bitmap);
  //GRect headsize = gbitmap_get_bounds(s_head_bitmap);
  //APP_LOG(APP_LOG_LEVEL_INFO, "Head Bytes per row: %d (%d, %d)", bytes, headsize.size.w, headsize.size.h);
  //format = gbitmap_get_format(s_head_bitmap);
  //APP_LOG(APP_LOG_LEVEL_INFO, "Head Bitmap format: %d", (int)format);
  head = create_ball(s_head_bitmap, headradius, headcenterx, headcentery);

  s_simple_bg_layer = layer_create(bounds);
  layer_set_update_proc(s_simple_bg_layer, bg_update_proc);
  layer_add_child(window_get_root_layer(window), s_simple_bg_layer);
  spin_globe(ANIMATION_INITIAL_DELAY, 1);
}

void update_globe() {
  GRect bounds = layer_get_unobstructed_bounds(window_layer);
  layer_set_bounds(s_simple_bg_layer, bounds);
  globeradius = bounds.size.h * 2 / 7;
  APP_LOG(APP_LOG_LEVEL_INFO, "globeradius: %d", globeradius);
  if (globeradius > maxgloberadius) globeradius = maxgloberadius;
  globecenterx = headcenterx = bounds.size.w / 2;
  globecentery = bounds.size.h / 2 + 20;
  headradius = bounds.size.h / 6;
  APP_LOG(APP_LOG_LEVEL_INFO, "headradius: %d", headradius);
  #ifdef PBL_RECT
  //if (globecentery > 60) globecentery += 10;
  #endif
  headcentery = globecentery - globeradius + 0;
  xres = bounds.size.w;
  yres = bounds.size.h;

  update_ball(body, globeradius, globecenterx, globecentery);
  update_ball(head, headradius, headcenterx, headcentery);
  layer_mark_dirty(s_simple_bg_layer);
}

void destroy_globe() {
  destroy_ball(body);
  destroy_ball(head);
  gbitmap_destroy(s_body_bitmap);
  gbitmap_destroy(s_head_bitmap);
  layer_destroy(s_simple_bg_layer);
}
