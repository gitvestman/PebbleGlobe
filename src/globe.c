#include <pebble.h>
#include "globe.h"
#include "message.h"

#define FIXED_360_DEG 0x10000
#define FIXED_360_DEG_SHIFT 16
#define FIXED_180_DEG 0x8000
#define FIXED_90_DEG 0x4000
#define BACK_COLOR 0xC1

static GBitmap *s_background_bitmap;
static Layer *s_simple_bg_layer;
static uint8_t* raw_bitmap_data;
static uint_fast8_t globeradius = 60;
static uint_fast16_t globeradiusx2 = 60*60;
static uint_fast8_t globecenterx, globecentery;
static uint16_t globelong = 90;
static int globelat = 0x1000;
static int xres = 0;
static int yres = 0;
static int sunlong = 0;
static int sunlat = 0;
static int animation_direction = 1;
static bool animating = true;
static bool gpsposition = 0;

static void update_animaion_parameters();
static uint8_t* framebufferdata;
static uint8_t framebuffer_bytes_per_row;

#define DRAW_BW_PIXEL( framebuffer, x, y, color ) \
      (((uint8_t*)(framebuffer->addr))[(y)*framebuffer->row_size_bytes + ((x) >> 3)] |= ((color) << ((x) & 0x07)));

#define DRAW_COLOR_PIXEL( framebuffer, x, y, color ) \
      (framebufferdata[(y)*framebuffer_bytes_per_row + (x)] = color);
      //(gbitmap_get_data(framebuffer)[(y)*gbitmap_get_bytes_per_row(framebuffer) + (x)] = color);

static int32_t arccos[81];
static int8_t sqrt_lookup[4500];

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
  for (int i = 0; i < 4500; i++) {
    sqrt_lookup[i] = sqrt_fast(i);
  }
}

void set_sun_position(uint16_t longitude, uint16_t latitude) {
  sunlong = longitude;
  sunlat = latitude;
  if (animating) {
    update_animaion_parameters();
  }
}

static void bg_update_proc(Layer *layer, GContext *ctx) {
  //time_t seconds1, seconds2;
  //uint16_t ms1, ms2;
    
  //time_ms(&seconds1, &ms1);
  if (!animating) {
    globelong = sunlong;
    globelat = sunlat;
  }
  
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
    
  graphics_context_set_stroke_color(ctx, GColorWhite);
  GBitmap *framebuffer = graphics_capture_frame_buffer(ctx);
  framebufferdata = gbitmap_get_data(framebuffer);
  framebuffer_bytes_per_row = gbitmap_get_bytes_per_row(framebuffer);
  int cosglobelat = cos_lookup(globelat);
  int singlobelat = sin_lookup(globelat);
  int bitmapwidth = gbitmap_get_bytes_per_row(s_background_bitmap);
  
  for (uint_fast8_t y = globecentery - globeradius; y < globecentery + globeradius; y++) {
    bool firstx = false;
    int width = globeradius;
    int ydiff = abs(y - globecentery);
    uint_fast16_t originallatitude = (y > globecentery ? 
      FIXED_180_DEG - arccos[ydiff] : arccos[ydiff]); 
    int sinlatglobe = ((globeradius * sin_lookup(originallatitude)) >> FIXED_360_DEG_SHIFT);
    int cordz = globecentery - y;
    int cordzsinglobelat = singlobelat * cordz;
    int cordzcosglobelat = cosglobelat * cordz;

    for (uint_fast8_t x = globecenterx - globeradius; x < globecenterx + globeradius; x++) {
      int cordx = globecenterx - x;
      int xdiff = abs(cordx);
      uint_fast16_t radiusx2 = xdiff * xdiff + ydiff * ydiff;
      if (radiusx2 < globeradiusx2) {
        if (!firstx) { 
          firstx = true;
          width = cordx;
        }
        uint16_t longitude = (x > globecenterx ? 
          FIXED_180_DEG - arccos[xdiff * globeradius / width] :
          arccos[xdiff * globeradius / width]); 
        uint16_t latitude = originallatitude;

        if ((globelat & 0xFF00) != 0) {
          // Convert to cartesian coordinates (confusion since y on screeen is z in 3d system)
          int cordy = (sinlatglobe * sin_lookup(longitude)) >> FIXED_360_DEG_SHIFT;
          
          // Multiplication (rotation by the x-axis)
          // x'   | 1   0       0    | | x |     x' = x
          // y' = | 0 cos(t)  sin(t) | | y | =>  y' = cos(t)*y + sin(t)*z
          // z'   | 0 -sin(t) cos(t) | | z |     z' = -sin(t)*y + cos(t)*z
          int xrot = cordx;
          int yrot = (cosglobelat * cordy + cordzsinglobelat) >> FIXED_360_DEG_SHIFT;
          int zrot = (-singlobelat * cordy + cordzcosglobelat) >> FIXED_360_DEG_SHIFT;
        
          // convert to spherical coordinates
          latitude = atan2_lookup(sqrt_lookup[xrot * xrot + yrot * yrot], zrot);
          longitude = atan2_lookup(yrot, xrot);
        }
        // Rotate longitude
        longitude += globelong;

        uint_fast8_t lineposition = ((latitude * 180) >> FIXED_360_DEG_SHIFT) * bitmapwidth;
#ifdef PBL_COLOR
        uint16_t byteposition = lineposition + ((longitude * 180) >> FIXED_360_DEG_SHIFT);
        uint8_t pixel = raw_bitmap_data[byteposition];
        DRAW_COLOR_PIXEL(framebuffer, x, y, pixel);
#else        
        uint16_t byteposition = lineposition + ((longitude * 180) >> (FIXED_360_DEG_SHIFT + 3));
        uint8_t byte = raw_bitmap_data[byteposition];
        uint8_t pixel = (byte >> (((longitude * 180) >> FIXED_360_DEG_SHIFT) & 0x07)) & 1;
        DRAW_BW_PIXEL(framebuffer, x, y, pixel);
#endif
      }
    }
  }
  if (false && currentlong != 0) {
    int sinlatglobe = ((globeradius * sin_lookup(currentlat)) >> FIXED_360_DEG_SHIFT);
    uint16_t rotatedlong = FIXED_90_DEG - currentlong + globelong;
    int z = (globeradius * cos_lookup(currentlat)) >> FIXED_360_DEG_SHIFT;
    int x = (sinlatglobe * cos_lookup(rotatedlong)) >> FIXED_360_DEG_SHIFT;
    int y = (sinlatglobe * sin_lookup(rotatedlong)) >> FIXED_360_DEG_SHIFT;
    int myx = x + globecenterx;
    int myy = (cosglobelat * y + singlobelat * z) >> FIXED_360_DEG_SHIFT;
    int myz = ((-singlobelat * y + cosglobelat * z) >> FIXED_360_DEG_SHIFT) + globecentery;
    if (myy > 0 && gpsposition) {
#ifdef PBL_COLOR
      DRAW_COLOR_PIXEL(framebuffer, myx, myz, 0xF0);
      DRAW_COLOR_PIXEL(framebuffer, myx, myz+1, 0xF0);
      DRAW_COLOR_PIXEL(framebuffer, myx+1, myz, 0xF0);
      DRAW_COLOR_PIXEL(framebuffer, myx+1, myz+1, 0xF0);
      DRAW_COLOR_PIXEL(framebuffer, myx, myz-1, 0xF0);
      DRAW_COLOR_PIXEL(framebuffer, myx-1, myz, 0xF0);
      DRAW_COLOR_PIXEL(framebuffer, myx-1, myz-1, 0xF0);
      DRAW_COLOR_PIXEL(framebuffer, myx-1, myz+1, 0xF0);
      DRAW_COLOR_PIXEL(framebuffer, myx+1, myz-1, 0xF0);
#else
      uint8_t pixel = gpsposition;
      DRAW_BW_PIXEL(framebuffer, myx, myz, pixel);
      DRAW_BW_PIXEL(framebuffer, myx, myz+1, pixel);
      DRAW_BW_PIXEL(framebuffer, myx+1, myz, pixel);
      DRAW_BW_PIXEL(framebuffer, myx+1, myz+1, pixel);
      DRAW_BW_PIXEL(framebuffer, myx, myz-1, pixel);
      DRAW_BW_PIXEL(framebuffer, myx-1, myz, pixel);
      DRAW_BW_PIXEL(framebuffer, myx-1, myz-1, pixel);
      DRAW_BW_PIXEL(framebuffer, myx-1, myz+1, pixel);
      DRAW_BW_PIXEL(framebuffer, myx+1, myz-1, pixel);
#endif
    }
    gpsposition = !gpsposition;
  }
  graphics_release_frame_buffer(ctx, framebuffer);
  //time_ms(&seconds2, &ms2);
  //int diff = (seconds2 - seconds1) * 1000 + (ms2 - ms1);
  //APP_LOG(APP_LOG_LEVEL_INFO, "Redering time %d", diff);
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
}

static void update_animaion_parameters() {
  longitude_length = abs(sunlong - longitude_start) + FIXED_360_DEG;
}

static void anim_stopped_handler(Animation* anim, bool finished, void* context) {
  animation_destroy(anim);
  animating = false;
}

static void anim_update_handler(Animation* anim, AnimationProgress progress) {
  globelong = longitude_start + animation_direction * longitude_length * progress / ANIMATION_NORMALIZED_MAX;
  globelat = latitude_start + latitude_length * progress / ANIMATION_NORMALIZED_MAX;
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
  layer_destroy(s_simple_bg_layer);
}

