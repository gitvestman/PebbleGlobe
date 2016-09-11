#include <pebble.h>
#include "ball.h"

#define FIXED_360_DEG 0x10000
#define FIXED_360_DEG_SHIFT 16
#define FIXED_180_DEG 0x8000
#define FIXED_90_DEG 0x4000

struct ball {
  GBitmap *bitmap;
  uint8_t* bitmap_data;
  int bitmapwidth;
  GBitmapFormat format;
  GRect bitmapbounds;
  int bitmapsize;
  int8_t radius;
  uint_fast16_t radiusx2;
  uint_fast8_t centerx, centery;
  int32_t *arccos;
#ifdef PBL_COLOR
  GColor* palette;
#endif
};

#define DRAW_BW_PIXEL( framebuffer, x, yoffset, color ) \
      (color != 0 ? (framebufferdata[yoffset + ((x) >> 3)] |= ((color) << ((x) & 0x07))) : \
                    (framebufferdata[yoffset + ((x) >> 3)] &= ~((1) << ((x) & 0x07))));

#define DRAW_COLOR_PIXEL( framebuffer, x, yoffset, color ) \
      (framebufferdata[yoffset + (x)] = color);

#define DRAW_ROUND_PIXEL( rowdata, x, color ) \
      (rowdata[x] = color);

static int8_t sqrt_lookup[4500];

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

void init_sqrt() {
  for (int i = 0; i < 4500; i++) {
    sqrt_lookup[i] = sqrt_fast(i);
  }
}

static void init_arccos(Ball ball) {
  // 0 to 90 deegreed in 0.5 degree steps
  for (int a = 0; a < FIXED_90_DEG; a+=91) {
    int32_t x = (FIXED_180_DEG + cos_lookup(a) * ball->radius) / FIXED_360_DEG;
    ball->arccos[x] = a;
  }
}

Ball create_ball(GBitmap *bitmap, int radius, int x, int y) {
  Ball ball = malloc(sizeof(struct ball));
  APP_LOG(APP_LOG_LEVEL_INFO, "Ball allocated %d bytes, position(%d, %d)", (int)sizeof(struct ball), x, y);
  ball->bitmap_data = gbitmap_get_data(bitmap);
  ball->bitmapwidth = gbitmap_get_bytes_per_row(bitmap);
  ball->bitmapbounds = gbitmap_get_bounds(bitmap);
  ball->format = gbitmap_get_format(bitmap);
  ball->bitmapsize = ball->bitmapwidth * ball->bitmapbounds.size.h;
  ball->radius = radius;
  ball->radiusx2 = radius * radius;
  ball->centerx = x;
  ball->centery = y;
  ball->arccos = malloc(sizeof(int32_t) * 81);
  init_arccos(ball);
#ifdef PBL_COLOR
  ball->palette = gbitmap_get_palette(bitmap);
#endif
  return ball;
}

void destroy_ball(Ball ball) {
  free(ball->arccos);
  free(ball);
}

void ball_update_proc(Ball ball, Layer *layer, GContext *ctx, int latitude_rotation, int longitude_rotation)
{
  graphics_context_set_stroke_color(ctx, GColorWhite);
  GBitmap *framebuffer = graphics_capture_frame_buffer(ctx);
  GRect bounds = gbitmap_get_bounds(framebuffer);
#ifndef PBL_ROUND
  uint8_t* framebufferdata = gbitmap_get_data(framebuffer);
  uint8_t framebuffer_bytes_per_row = gbitmap_get_bytes_per_row(framebuffer);
#endif
  int coslat = cos_lookup(latitude_rotation);
  int sinlat = sin_lookup(latitude_rotation);
  uint_fast8_t starty = ((int)ball->centery - ball->radius) < 0 ? 0 : ball->centery - ball->radius;
  uint_fast8_t startx = ((int)ball->centerx - ball->radius) < 0 ? 0 : ball->centerx - ball->radius;
  uint_fast8_t stopy = ((int)ball->centery + ball->radius) >= bounds.size.h ? (uint_fast8_t)bounds.size.h : (uint_fast8_t)(ball->centery + ball->radius);
  uint_fast8_t stopx = ((int)ball->centerx + ball->radius) >= bounds.size.w ? (uint_fast8_t)bounds.size.w : (uint_fast8_t)(ball->centerx + ball->radius);

  for (uint_fast8_t y = starty; y < stopy; y++) {
#ifdef PBL_ROUND
    GBitmapDataRowInfo info = gbitmap_get_data_row_info(framebuffer, y);
    if ((int)startx < info.min_x) startx = info.min_x;
    if ((int)stopx > info.max_x) stopx = info.max_x;
#else
    uint_fast16_t yoffset = y*framebuffer_bytes_per_row;
#endif
    bool firstx = false;
    int width = ball->radius;
    int ydiff = abs(y - ball->centery);
    uint_fast16_t originallatitude = (y > ball->centery ?
      FIXED_180_DEG - ball->arccos[ydiff] : ball->arccos[ydiff]);
    int sinlathead = ((ball->radius * sin_lookup(originallatitude)) >> FIXED_360_DEG_SHIFT);
    int cordz = ball->centery - y;
    int cordzsinlat = sinlat * cordz;
    int cordzcoslat = coslat * cordz;

    for (uint_fast8_t x = startx; x < stopx; x++) {
      int cordx = ball->centerx - x;
      int xdiff = abs(cordx);
      uint_fast16_t radiusx2 = xdiff * xdiff + ydiff * ydiff;
      if (radiusx2 < ball->radiusx2) {
        if (!firstx) {
          firstx = true;
          width = cordx;
        }
        uint16_t longitude = (x > ball->centerx ?
          FIXED_180_DEG - ball->arccos[xdiff * ball->radius / width] :
          ball->arccos[xdiff * ball->radius / width]);
        uint16_t latitude = originallatitude;

        if ((latitude_rotation & 0xFF00) != 0) {
          // Convert to cartesian coordinates (confusion since y on screeen is z in 3d system)
          int cordy = (sinlathead * sin_lookup(longitude)) >> FIXED_360_DEG_SHIFT;

          // Multiplication (rotation by the x-axis)
          // x'   | 1   0       0    | | x |     x' = x
          // y' = | 0 cos(t)  sin(t) | | y | =>  y' = cos(t)*y + sin(t)*z
          // z'   | 0 -sin(t) cos(t) | | z |     z' = -sin(t)*y + cos(t)*z
          int xrot = cordx;
          int yrot = (coslat * cordy + cordzsinlat) >> FIXED_360_DEG_SHIFT;
          int zrot = (-sinlat * cordy + cordzcoslat) >> FIXED_360_DEG_SHIFT;

          // convert to spherical coordinates
          latitude = atan2_lookup(sqrt_lookup[xrot * xrot + yrot * yrot], zrot);
          longitude = atan2_lookup(yrot, xrot);
        }
        // Rotate longitude
        longitude += longitude_rotation;

        uint_fast8_t lineposition = ((latitude * ball->bitmapbounds.size.w) >> FIXED_360_DEG_SHIFT) * ball->bitmapwidth;
        uint_fast16_t rowposition = ((longitude * ball->bitmapbounds.size.w) >> FIXED_360_DEG_SHIFT);
        uint8_t pixel = 0;
#ifdef PBL_COLOR

        if (ball->format == GBitmapFormat8Bit) {
          uint16_t byteposition = lineposition + rowposition;
          if (byteposition < ball->bitmapsize) {
            pixel = ball->bitmap_data[byteposition];
          }
        } else if (ball->format == GBitmapFormat4BitPalette) {
          uint16_t byteposition = lineposition + (rowposition >> 1);
          if (byteposition < ball->bitmapsize) {
            uint8_t byte = ball->bitmap_data[byteposition];
            pixel = ball->palette[(byte >> (1 - (rowposition & 0x01)) * 4) & 0x0F].argb;
          }
        } else if (ball->format == GBitmapFormat2BitPalette) {
          uint16_t byteposition = lineposition + (rowposition >> 2);
          if (byteposition < ball->bitmapsize) {
            uint8_t byte = ball->bitmap_data[byteposition];
            pixel = ball->palette[(byte >> (3 - (rowposition & 0x03)) * 2) & 0x03].argb;
          }
        }
#else
        uint16_t byteposition = lineposition + (rowposition >> 3);
        if (byteposition < ball->bitmapsize) {
          uint8_t byte = ball->bitmap_data[byteposition];
          pixel = (byte >> (rowposition & 0x07)) & 1;
        }
#endif

#ifdef PBL_ROUND
        /*if (x == ball->centerx)
          APP_LOG(APP_LOG_LEVEL_INFO, "Round Draw position(%d, %d)", x, y);*/
        DRAW_ROUND_PIXEL(info.data, x, pixel);
#elif PBL_COLOR
        DRAW_COLOR_PIXEL(framebuffer, x, yoffset, pixel);
#else
        DRAW_BW_PIXEL(framebuffer, x, yoffset, pixel);
#endif
      }
    }
  }
  graphics_release_frame_buffer(ctx, framebuffer);
}

void draw_gps_position(Ball ball, Layer *layer, GContext *ctx,
  int latitude_rotation, int longitude_rotation,
  uint16_t longitude, uint16_t latitude)
{
  GBitmap *framebuffer = graphics_capture_frame_buffer(ctx);
#ifndef PBL_ROUND
  uint8_t* framebufferdata = gbitmap_get_data(framebuffer);
  uint8_t framebuffer_bytes_per_row = gbitmap_get_bytes_per_row(framebuffer);
#endif

  int coslat = cos_lookup(latitude_rotation);
  int sinlat = sin_lookup(latitude_rotation);

  int sinlathead = ((ball->radius * sin_lookup(latitude)) >> FIXED_360_DEG_SHIFT);
  uint16_t rotatedlong = FIXED_90_DEG - longitude + longitude_rotation;
  int z = (ball->radius * cos_lookup(latitude)) >> FIXED_360_DEG_SHIFT;
  int x = (sinlathead * cos_lookup(rotatedlong)) >> FIXED_360_DEG_SHIFT;
  int y = (sinlathead * sin_lookup(rotatedlong)) >> FIXED_360_DEG_SHIFT;
  int myx = x + ball->centerx;
  int myy = (coslat * y + sinlat * z) >> FIXED_360_DEG_SHIFT;
  int myz = ((-sinlat * y + coslat * z) >> FIXED_360_DEG_SHIFT) + ball->centery;
  if (myy > 0) {
    for (int dz = -1; dz <= 1; dz++) {
#ifdef PBL_ROUND
      GBitmapDataRowInfo info = gbitmap_get_data_row_info(framebuffer, myz + dz);
#else
      int yoffset = (myz + dz) * framebuffer_bytes_per_row;
#endif
      for (int dx = -1; dx <= 1; dx++) {
#ifdef PBL_ROUND
        DRAW_ROUND_PIXEL(info.data, myx + dx, 0xF0);
#elif PBL_COLOR
        DRAW_COLOR_PIXEL(framebuffer, myx + dx, yoffset, 0xF0);
#else
        uint8_t pixel = 1;
        DRAW_BW_PIXEL(framebuffer, myx + dx, yoffset, pixel);
#endif
      }
    }
  }
  graphics_release_frame_buffer(ctx, framebuffer);
}
