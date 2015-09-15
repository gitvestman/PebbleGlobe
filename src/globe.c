#include <pebble.h>
#include "clock.h"
#include "globe.h"
#include "ball.h"

static GBitmap *s_body_bitmap;
static GBitmap *s_head_bitmap;
static Layer *s_simple_bg_layer;

static Ball body;
static Ball head;

static int8_t globeradius = 48;
static uint_fast8_t globecenterx, globecentery;

static int8_t headradius = 28;
static uint_fast8_t headcenterx, headcentery;

static uint16_t globelong = 90;
static int globelat = 0x2000;

static uint16_t headlong = 90;
static int headlat = -0x1000;
static int headbump = 0;

static int xres = 0;
static int yres = 0;
static int sunlong = 0xA000;
static int animation_direction = 1;
static bool animating = true;
static int animation_count;

#define FIXED_360_DEG_SHIFT 16
#define FIXED_360_DEG 0x10000

// static void draw_main_globe(Layer *layer, GContext *ctx, uint8_t* raw_bitmap_data) {
//
//   GBitmapFormat format = gbitmap_get_format(s_body_bitmap);
//   #ifdef PBL_COLOR
//   GColor* palette = gbitmap_get_palette(s_body_bitmap);
//   #endif
//   int bitmapwidth = gbitmap_get_bytes_per_row(s_body_bitmap);
//
//   graphics_context_set_stroke_color(ctx, GColorWhite);
//   GBitmap *framebuffer = graphics_capture_frame_buffer(ctx);
//   framebufferdata = gbitmap_get_data(framebuffer);
//   framebuffer_bytes_per_row = gbitmap_get_bytes_per_row(framebuffer);
//   int cosglobelat = cos_lookup(globelat);
//   int singlobelat = sin_lookup(globelat);
//
//   for (uint_fast8_t y = globecentery - globeradius; y < globecentery + globeradius; y++) {
//     uint_fast16_t yoffset = y*framebuffer_bytes_per_row;
//     bool firstx = false;
//     int width = globeradius;
//     int ydiff = abs(y - globecentery);
//     uint_fast16_t originallatitude = (y > globecentery ?
//       FIXED_180_DEG - arccos_globe[ydiff] : arccos_globe[ydiff]);
//     int sinlatglobe = ((globeradius * sin_lookup(originallatitude)) >> FIXED_360_DEG_SHIFT);
//     int cordz = globecentery - y;
//     int cordzsinglobelat = singlobelat * cordz;
//     int cordzcosglobelat = cosglobelat * cordz;
//
//     for (uint_fast8_t x = globecenterx - globeradius; x < globecenterx + globeradius; x++) {
//       int cordx = globecenterx - x;
//       int xdiff = abs(cordx);
//       uint_fast16_t radiusx2 = xdiff * xdiff + ydiff * ydiff;
//       if (radiusx2 < globeradiusx2) {
//         if (!firstx) {
//           firstx = true;
//           width = cordx;
//         }
//         uint16_t longitude = (x > globecenterx ?
//           FIXED_180_DEG - arccos_globe[xdiff * globeradius / width] :
//           arccos_globe[xdiff * globeradius / width]);
//         uint16_t latitude = originallatitude;
//
//         if ((globelat & 0xFF00) != 0) {
//           // Convert to cartesian coordinates (confusion since y on screeen is z in 3d system)
//           int cordy = (sinlatglobe * sin_lookup(longitude)) >> FIXED_360_DEG_SHIFT;
//
//           // Multiplication (rotation by the x-axis)
//           // x'   | 1   0       0    | | x |     x' = x
//           // y' = | 0 cos(t)  sin(t) | | y | =>  y' = cos(t)*y + sin(t)*z
//           // z'   | 0 -sin(t) cos(t) | | z |     z' = -sin(t)*y + cos(t)*z
//           int xrot = cordx;
//           int yrot = (cosglobelat * cordy + cordzsinglobelat) >> FIXED_360_DEG_SHIFT;
//           int zrot = (-singlobelat * cordy + cordzcosglobelat) >> FIXED_360_DEG_SHIFT;
//
//           // convert to spherical coordinates
//           latitude = atan2_lookup(sqrt_lookup[xrot * xrot + yrot * yrot], zrot);
//           longitude = atan2_lookup(yrot, xrot);
//         }
//         // Rotate longitude
//         longitude += globelong;
//
//         uint_fast8_t lineposition = ((latitude * 256) >> FIXED_360_DEG_SHIFT) * bitmapwidth;
//         uint_fast16_t rowposition = ((longitude * 256) >> FIXED_360_DEG_SHIFT);
// #ifdef PBL_COLOR
//
//         uint8_t pixel = 0;
//         if (format == GBitmapFormat8Bit) {
//           uint16_t byteposition = lineposition + rowposition;
//           pixel = raw_bitmap_data[byteposition];
//         } else if (format == GBitmapFormat4BitPalette) {
//           uint16_t byteposition = lineposition + (rowposition >> 1);
//           uint8_t byte = raw_bitmap_data[byteposition];
//           pixel = palette[(byte >> (1 - (rowposition & 0x01)) * 4) & 0x0F].argb;
//         }
//         DRAW_COLOR_PIXEL(framebuffer, x, yoffset, pixel);
// #else
//         uint16_t byteposition = lineposition + (rowposition >> 3);
//         if (byteposition > 4096) { byteposition &= 0x3FF; }
//         uint8_t byte = raw_bitmap_data[byteposition];
//         uint8_t pixel = (byte >> (rowposition & 0x07)) & 1;
//         DRAW_BW_PIXEL(framebuffer, x, yoffset, pixel);
// #endif
//       }
//     }
//   }
//   graphics_release_frame_buffer(ctx, framebuffer);
//   //time_ms(&seconds2, &ms2);
//   //int diff = (seconds2 - seconds1) * 1000 + (ms2 - ms1);
//   //APP_LOG(APP_LOG_LEVEL_INFO, "%d:%d Redering time %d", (int)seconds2, (int)ms2, diff);
// }
//
// static void draw_head(Layer *layer, GContext *ctx, uint8_t* raw_bitmap_data) {
//   GBitmapFormat format = gbitmap_get_format(s_head_bitmap);
//   #ifdef PBL_COLOR
//   GColor* palette = gbitmap_get_palette(s_head_bitmap);
//   #endif
//   int bitmapwidth = gbitmap_get_bytes_per_row(s_head_bitmap);
//
//   graphics_context_set_stroke_color(ctx, GColorWhite);
//   GBitmap *framebuffer = graphics_capture_frame_buffer(ctx);
//   framebufferdata = gbitmap_get_data(framebuffer);
//   framebuffer_bytes_per_row = gbitmap_get_bytes_per_row(framebuffer);
//   int cosheadlat = cos_lookup(headlat);
//   int sinheadlat = sin_lookup(headlat);
//
//   for (uint_fast8_t y = (headcentery + headbump) - headradius; y < (headcentery + headbump) + (headradius / 2); y++) {
//     uint_fast16_t yoffset = y*framebuffer_bytes_per_row;
//     bool firstx = false;
//     int width = headradius;
//     int ydiff = abs(y - (headcentery + headbump));
//     uint_fast16_t originallatitude = (y > (headcentery + headbump) ?
//       FIXED_180_DEG - arccos_head[ydiff] : arccos_head[ydiff]);
//     int sinlathead = ((headradius * sin_lookup(originallatitude)) >> FIXED_360_DEG_SHIFT);
//     int cordz = (headcentery + headbump) - y;
//     int cordzsinheadlat = sinheadlat * cordz;
//     int cordzcosheadlat = cosheadlat * cordz;
//
//     for (uint_fast8_t x = headcenterx - headradius; x < headcenterx + headradius; x++) {
//       int cordx = headcenterx - x;
//       int xdiff = abs(cordx);
//       uint_fast16_t radiusx2 = xdiff * xdiff + ydiff * ydiff;
//       if (radiusx2 < headradiusx2) {
//         if (!firstx) {
//           firstx = true;
//           width = cordx;
//         }
//         uint16_t longitude = (x > headcenterx ?
//           FIXED_180_DEG - arccos_head[xdiff * headradius / width] :
//           arccos_head[xdiff * headradius / width]);
//         uint16_t latitude = originallatitude;
//
//         if ((headlat & 0xFF00) != 0) {
//           // Convert to cartesian coordinates (confusion since y on screeen is z in 3d system)
//           int cordy = (sinlathead * sin_lookup(longitude)) >> FIXED_360_DEG_SHIFT;
//
//           // Multiplication (rotation by the x-axis)
//           // x'   | 1   0       0    | | x |     x' = x
//           // y' = | 0 cos(t)  sin(t) | | y | =>  y' = cos(t)*y + sin(t)*z
//           // z'   | 0 -sin(t) cos(t) | | z |     z' = -sin(t)*y + cos(t)*z
//           int xrot = cordx;
//           int yrot = (cosheadlat * cordy + cordzsinheadlat) >> FIXED_360_DEG_SHIFT;
//           int zrot = (-sinheadlat * cordy + cordzcosheadlat) >> FIXED_360_DEG_SHIFT;
//
//           // convert to spherical coordinates
//           latitude = atan2_lookup(sqrt_lookup[xrot * xrot + yrot * yrot], zrot);
//           longitude = atan2_lookup(yrot, xrot);
//         }
//         // Rotate longitude
//         longitude += headlong;
//
//         uint_fast8_t lineposition = ((latitude * 256) >> FIXED_360_DEG_SHIFT) * bitmapwidth;
//         uint_fast16_t rowposition = ((longitude * 256) >> FIXED_360_DEG_SHIFT);
// #ifdef PBL_COLOR
//
//         uint8_t pixel = 0;
//         if (format == GBitmapFormat8Bit) {
//           uint16_t byteposition = lineposition + rowposition;
//           pixel = raw_bitmap_data[byteposition];
//         } else if (format == GBitmapFormat4BitPalette) {
//           uint16_t byteposition = lineposition + (rowposition >> 1);
//           uint8_t byte = raw_bitmap_data[byteposition];
//           pixel = palette[(byte >> (1 - (rowposition & 0x01)) * 4) & 0x0F].argb;
//         }
//         DRAW_COLOR_PIXEL(framebuffer, x, yoffset, pixel);
// #else
//         uint16_t byteposition = lineposition + (rowposition >> 3);
//         uint8_t byte = raw_bitmap_data[byteposition];
//         uint8_t pixel = (byte >> (rowposition & 0x07)) & 1;
//         DRAW_BW_PIXEL(framebuffer, x, yoffset, pixel);
// #endif
//       }
//     }
//   }
//   graphics_release_frame_buffer(ctx, framebuffer);
//   //time_ms(&seconds2, &ms2);
//   //int diff = (seconds2 - seconds1) * 1000 + (ms2 - ms1);
//   //APP_LOG(APP_LOG_LEVEL_INFO, "%d:%d Redering time %d", (int)seconds2, (int)ms2, diff);
// }

static void bg_update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);

  if (!animating) {
    globelat -= 128 * animation_direction;
    globelong += 64 * animation_direction;
    headlong += 256 * animation_direction;
    if (headlong < 50 || headlong > 20000)
      animation_direction = -animation_direction;
  }

  void ball_update_proc(Ball ball, Layer *layer, GContext *ctx,
    int latitude_rotation, int longitude_rotation, int xdelta, int ydelta);

  ball_update_proc(body, layer, ctx, globelat, globelong, 0, 0);
  ball_update_proc(head, layer, ctx, headlat, headlong, 0, headbump);
  //draw_main_globe(layer, ctx, raw_bitmap_globe_data);
  //draw_head(layer, ctx, raw_bitmap_head_data);
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
  animation_count = 0;
  tick_timer_service_unsubscribe();
}

static void anim_stopped_handler(Animation* anim, bool finished, void* context) {
  animation_destroy(anim);
  animating = false;
  reset_ticks();
  APP_LOG(APP_LOG_LEVEL_INFO, "Animation count %d", animation_count);
}

static void anim_update_handler(Animation* anim, AnimationProgress progress) {
  globelong = longitude_start + animation_direction * longitude_length * progress / ANIMATION_NORMALIZED_MAX;
  globelat = latitude_start + animation_direction * latitude_length * progress / ANIMATION_NORMALIZED_MAX;
  headlong = headlong_start + animation_direction * sin_lookup(progress * 3) / 8;
  headbump = abs(3 * cos_lookup(progress * 8) >> FIXED_360_DEG_SHIFT);
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

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  globecenterx = headcenterx = bounds.size.w / 2;
  globecentery = bounds.size.h / 2 + 20;
  headcentery = globecentery - globeradius + 4;
  xres = bounds.size.w;
  yres = bounds.size.h;
  // Main globe
  s_body_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_GLOBE);
  int bytes = gbitmap_get_bytes_per_row(s_body_bitmap);
  GRect bodysize = gbitmap_get_bounds(s_body_bitmap);
  APP_LOG(APP_LOG_LEVEL_INFO, "Globe Bytes per row: %d (%d, %d)", bytes, bodysize.size.w, bodysize.size.h);
  GBitmapFormat format = gbitmap_get_format(s_body_bitmap);
  APP_LOG(APP_LOG_LEVEL_INFO, "Globe Bitmap format: %d", (int)format);
  //raw_bitmap_globe_data = gbitmap_get_data(s_body_bitmap);
  body = create_ball(s_body_bitmap, globeradius, globecenterx, globecentery);
  APP_LOG(APP_LOG_LEVEL_INFO, "Body created: %p", body);

  // Head
  s_head_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HEAD);
  bytes = gbitmap_get_bytes_per_row(s_head_bitmap);
  GRect headsize = gbitmap_get_bounds(s_head_bitmap);
  APP_LOG(APP_LOG_LEVEL_INFO, "Head Bytes per row: %d (%d, %d)", bytes, headsize.size.w, headsize.size.h);
  format = gbitmap_get_format(s_head_bitmap);
  APP_LOG(APP_LOG_LEVEL_INFO, "Head Bitmap format: %d", (int)format);
  //raw_bitmap_head_data = gbitmap_get_data(s_head_bitmap);
  head = create_ball(s_head_bitmap, headradius, headcenterx, headcentery);

  s_simple_bg_layer = layer_create(bounds);
  layer_set_update_proc(s_simple_bg_layer, bg_update_proc);
  layer_add_child(window_get_root_layer(window), s_simple_bg_layer);
  spin_globe(ANIMATION_INITIAL_DELAY, 1);
}

void destroy_globe() {
  destroy_ball(body);
  destroy_ball(head);
  gbitmap_destroy(s_body_bitmap);
  gbitmap_destroy(s_head_bitmap);
  layer_destroy(s_simple_bg_layer);
}
