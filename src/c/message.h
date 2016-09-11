#pragma once

typedef struct config {
  bool showDate;
  bool showSteps;
  bool animations;
  bool inverted;
} Config;

extern int currentlong;
extern int currentlat;
extern int16_t timezone_offset;
extern Config app_config;

void message_init();
void message_deinit();
