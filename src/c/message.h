#pragma once

typedef struct config {
  bool showDate;
  bool showSteps;
  bool animations;
  bool inverted;
  bool bold;
} Config;

extern int currentlong;
extern int currentlat;
extern int16_t timezone_offset;
extern Config app_config;

void message_init(Window *window);
void message_deinit();
