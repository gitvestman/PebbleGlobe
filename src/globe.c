#include <pebble.h>
#include "globe.h"

#define FIXED_360_DEG 0x10000
#define FIXED_360_DEG_SHIFT 16
#define FIXED_180_DEG 0x8000
#define FIXED_90_DEG 0x4000

static GBitmap *s_background_bitmap;
static Layer *s_simple_bg_layer;
static uint8_t* raw_bitmap_data;
static int globeradius = 60;
static int globeradiusx2 = 3600;
static int globecenterx, globecentery;
static uint16_t globelong = 90;
static int globelat = -0x1000;
static int xres = 0;
static int yres = 0;
static int animation_direction = 1;
static bool animating = false;

#define DRAW_BW_PIXEL( framebuffer, x, y, color ) \
      (((uint8_t*)(framebuffer->addr))[y*framebuffer->row_size_bytes + x / 8] |= ((color) << (x % 8)));

#define DRAW_COLOR_PIXEL( framebuffer, x, y, color ) \
      (gbitmap_get_data(framebuffer)[y*gbitmap_get_bytes_per_row(framebuffer) + x] = color);

static int32_t arccos[81];
static int32_t sqrt_lookup[4000];

static void init_arccos() {
  // 0 to 90 deegreed in 0.5 degree steps
  for (int a = 0; a < FIXED_90_DEG; a+=91) {
    int32_t x = (FIXED_180_DEG + cos_lookup(a) * globeradius) / FIXED_360_DEG;
    arccos[x] = a;
  }
}

// http://en.wikipedia.org/wiki/Fast_inverse_square_root
#define SQRT_MAGIC_F 0x5f3759df 
inline float  sqrt_fast(const float x){
  union
  {
    float x;
    int i;
  } u;
  u.x = x;
  u.i = SQRT_MAGIC_F - (u.i >> 1); 
  
  // newton step for increased accuracy
  return x * u.x * (1.5f - 0.5f * x * u.x * u.x);
}

static void init_sqrt() {
  for (int i = 0; i < 4000; i++) {
    sqrt_lookup[i] = sqrt_fast(i);
  }
}

static void bg_update_proc(Layer *layer, GContext *ctx) {
  time_t seconds1, seconds2;
  uint16_t ms1, ms2;
    
  time_ms(&seconds1, &ms1);
  if (!animating)
    globelong += animation_direction * 0x100;
  
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
  //GRect bounds = layer_get_bounds(layer);
  graphics_context_set_stroke_color(ctx, GColorWhite);
  GBitmap *framebuffer = graphics_capture_frame_buffer(ctx);
  int cosglobelat = cos_lookup(globelat);
  int singlobelat = sin_lookup(globelat);
  
  for (int y = globecentery - globeradius; y < globecentery + globeradius; y++) {
    bool firstx = false;
    int width = globeradius;
    int ydiff = abs(y - globecentery);
    int originallatitude = (y > globecentery ? 
      FIXED_180_DEG - arccos[ydiff] : arccos[ydiff]); 
    int sinlatglobe = ((globeradius * sin_lookup(originallatitude)) >> FIXED_360_DEG_SHIFT);

    for (int x = globecenterx - globeradius; x < globecenterx + globeradius; x++) {
      int xdiff = abs(x - globecenterx);
      int radiusx2 = xdiff * xdiff + ydiff * ydiff;
      if (radiusx2 < globeradiusx2) {
        if (!firstx) { 
          firstx = true;
          width = globecenterx - x;
        }
        uint16_t longitude = (x > globecenterx ? 
          FIXED_180_DEG - arccos[xdiff * globeradius / width] :
          arccos[xdiff * globeradius / width]); 
        int latitude = originallatitude;

        if (globelat != 0) {
          // Convert to cartesian coordinates (confusion since y on screeen is z in 3d system)
          int cordy = (sinlatglobe * sin_lookup(longitude)) >> FIXED_360_DEG_SHIFT;
          int cordx = globecenterx - x;
          int cordz = globecentery - y;
          
          // Multiplication (rotation by the x-axis)
          // x'   | 1   0       0    | | x |     x' = x
          // y' = | 0 cos(t)  sin(t) | | y | =>  y' = cos(t)*y + sin(t)*z
          // z'   | 0 -sin(t) cos(t) | | z |     z' = -sin(t)*y + cos(t)*z
          int xrot = cordx;
          int yrot = (cosglobelat * cordy + singlobelat * cordz) >> FIXED_360_DEG_SHIFT;
          int zrot = (-singlobelat * cordy + cosglobelat * cordz) >> FIXED_360_DEG_SHIFT;
        
          // convert to spherical coordinates
          latitude = atan2_lookup(sqrt_lookup[xrot * xrot + yrot * yrot], zrot);
          longitude = atan2_lookup(yrot, xrot);
        }
        longitude += globelong;

        int bitmapwidth = gbitmap_get_bytes_per_row(s_background_bitmap);
        uint16_t byteposition = ((latitude * 180) >> FIXED_360_DEG_SHIFT)  * bitmapwidth + 
          ((longitude * 180) >> FIXED_360_DEG_SHIFT);
#ifdef PBL_COLOR
        uint8_t pixel = raw_bitmap_data[byteposition];
        graphics_context_set_stroke_color(ctx, (GColor)pixel);
        DRAW_COLOR_PIXEL(framebuffer, x, y, pixel);
#else        
        uint8_t byte = raw_bitmap_data[((latitude * 180) >> FIXED_360_DEG_SHIFT) * bitmapwidth + 
                                       ((longitude * 180) >> (FIXED_360_DEG_SHIFT + 3))];
        uint8_t pixel = (byte >> (byteposition % 8)) & 1;
        graphics_context_set_stroke_color(ctx, (GColor)pixel);
        DRAW_BW_PIXEL(framebuffer, x, y, pixel);
#endif
      }
    }
  }
  graphics_release_frame_buffer(ctx, framebuffer);
  time_ms(&seconds2, &ms2);
  int diff = (seconds2 - seconds1) * 1000 + (ms2 - ms1);
  APP_LOG(APP_LOG_LEVEL_INFO, "Redering time %d", diff);
}

static int animation_start = 0;
static Animation* s_globe_animation;
#define ANIMATION_DURATION 5000
#define ANIMATION_INITIAL_DELAY 500

static void anim_started_handler(Animation* anim, void* context) {
  animation_start = globelong;
  animating = true;
}

static void anim_stopped_handler(Animation* anim, bool finished, void* context) {
  animation_destroy(anim);
  animating = false;
}

static void anim_update_handler(Animation* anim, AnimationProgress progress) {
  globelong = animation_start + animation_direction * 0x16000 * progress / ANIMATION_NORMALIZED_MAX;
  layer_mark_dirty(s_simple_bg_layer);
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
  init_arccos();
  init_sqrt();
  
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  globecenterx = bounds.size.w / 2;
  globecentery = bounds.size.h / 2 + 10;
  xres = bounds.size.w;
  yres = bounds.size.h;
  // Create GBitmap, then set to created BitmapLayer
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_GLOBE);
  int bytes = gbitmap_get_bytes_per_row(s_background_bitmap);
  APP_LOG(APP_LOG_LEVEL_INFO, "Bytes per row: %d", bytes);
  GBitmapFormat format = gbitmap_get_format(s_background_bitmap);
  APP_LOG(APP_LOG_LEVEL_INFO, "Bitmap format: %d", (int)format);
  raw_bitmap_data = gbitmap_get_data(s_background_bitmap);
  
  s_simple_bg_layer = layer_create(bounds);
  layer_set_update_proc(s_simple_bg_layer, bg_update_proc);
  layer_add_child(window_get_root_layer(window), s_simple_bg_layer);
  spin_globe(ANIMATION_INITIAL_DELAY, 1);
}

void destroy_globe() {
  gbitmap_destroy(s_background_bitmap);
  //gbitmap_destroy(s_background_bitmap2);
  layer_destroy(s_simple_bg_layer);
}

