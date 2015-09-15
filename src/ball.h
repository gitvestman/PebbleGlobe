#pragma once

typedef struct ball *Ball;

Ball create_ball(GBitmap *bitmap, int radius, int x, int y);
void destroy_ball(Ball ball);
void ball_update_proc(Ball ball, Layer *layer, GContext *ctx,
  int latitude_rotation, int longitude_rotation, int xdelta, int ydelta);

void init_sqrt();
