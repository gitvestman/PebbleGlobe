#pragma once

typedef struct config {
  bool showDate;
  bool showTime;
  bool animations;
  bool inverted;
  bool bold;
} Config;

extern Config app_config;
extern GColor background_color;

void message_init(Window *window);
void message_deinit();
