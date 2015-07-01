#pragma once

#define KEY_LONGITUDE 0
#define KEY_LATITUDE 1
#define KEY_TIMEZONE 2
  
extern uint_fast16_t currentlong;
extern uint_fast16_t currentlat;
extern int_fast16_t timezone_offset;

void message_init();
void message_deinit();