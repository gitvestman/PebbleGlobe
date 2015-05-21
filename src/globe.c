#include <pebble.h>
#include "globe.h"


static GBitmap *s_background_bitmap;
static Layer *s_simple_bg_layer;
static uint8_t* raw_bitmap_data;
static int globeradius = 60;
static int globeradiusx2 = 3600;
static int globecenterx, globecentery;
static int globelong = 90;
static int globelat = -10;
static int xres = 0;
static int yres = 0;

#define DRAW_BW_PIXEL( framebuffer, x, y, color ) \
      (((uint8_t*)(framebuffer->addr))[y*framebuffer->row_size_bytes + x / 8] |= ((color) << (x % 8)));

#define DRAW_COLOR_PIXEL( framebuffer, x, y, color ) \
      (gbitmap_get_data(framebuffer)[y*gbitmap_get_bytes_per_row(framebuffer) + x] = color);

static int32_t arccos[61];

static void init_arccos() {
  for (int a = 0; a < 16384; a+=91) {
    int32_t x = (32768 + cos_lookup(a) * globeradius) / 65536;
    arccos[x] = (a + 90) / 182;
  }
}

static void bg_update_proc(Layer *layer, GContext *ctx) {
  //time_t seconds1, seconds2;
  //uint16_t ms1, ms2;
    
  //time_ms(&seconds1, &ms1);
  globelong += 1;
  
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_stroke_color(ctx, GColorWhite);
  GBitmap *framebuffer = graphics_capture_frame_buffer(ctx);
  
  for (int y = 0; y < bounds.size.h; y++) {
    bool firstx = false;
    int width = globeradius;
    for (int x = 0; x < bounds.size.w; x++) {
      int xdiff = abs(x - globecenterx);
      int ydiff = abs(y - globecentery);
      int radiusx2 = xdiff * xdiff + ydiff * ydiff;
      if (radiusx2 < globeradiusx2) {
        if (!firstx) { 
          firstx = true;
          width = globecenterx - x;
        }
        int longitude = globelong + (x > globecenterx ? 
          180 - arccos[xdiff * globeradius / width] :
          arccos[xdiff * globeradius / width]);
        int latitude = globelat + (y > globecentery ? 
           180 - arccos[ydiff] : arccos[ydiff]);

        if (latitude < 0) { latitude = -latitude; longitude += 180; }
        if (latitude > 180) { latitude = 180-latitude; longitude += 180; }
        if (longitude < 0) longitude += 360;
        if (longitude > 360) longitude -= 360;
        int bitmapwidth = gbitmap_get_bytes_per_row(s_background_bitmap);
#ifdef PBL_COLOR
        uint8_t pixel = raw_bitmap_data[latitude/2 * bitmapwidth + longitude / 2];
        graphics_context_set_stroke_color(ctx, (GColor)pixel);
        DRAW_COLOR_PIXEL(framebuffer, x, y, pixel);
#else        
        uint8_t byte = raw_bitmap_data[latitude/2 * bitmapwidth + longitude / 2 / 8];
        uint8_t pixel = (byte >> ((latitude/2 * bitmapwidth + longitude / 2) % 8)) & 1;
        graphics_context_set_stroke_color(ctx, (GColor)pixel);
        DRAW_BW_PIXEL(framebuffer, x, y, pixel);
#endif
      }
    }
  }
  graphics_release_frame_buffer(ctx, framebuffer);
  //time_ms(&seconds2, &ms2);
  //int diff = (seconds2 - seconds2) * 1000 + (ms2 - ms1);
  //APP_LOG(APP_LOG_LEVEL_INFO, "Redering time %d", diff);
}

void init_globe(Window *window) {
  init_arccos();
  
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
}

void destroy_globe() {
  gbitmap_destroy(s_background_bitmap);
  //gbitmap_destroy(s_background_bitmap2);
  layer_destroy(s_simple_bg_layer);
}

