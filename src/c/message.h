#pragma once

typedef struct config {
  bool showDate;
  bool showHealth;
  bool animations;
  bool inverted;
  bool bold;
  bool showBattery;
  bool center;
} Config;

extern int currentlong;
extern int currentlat;
extern int16_t timezone_offset;
extern Config app_config;
extern GColor background_color;

void message_init(Window *window);
void message_deinit();
