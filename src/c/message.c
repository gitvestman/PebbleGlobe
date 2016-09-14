#include <pebble.h>
#include "message.h"
#include "clock.h"

Layer* root_layer;
int currentlong;
int currentlat;
int16_t timezone_offset;
Config app_config = { .showDate = true, .showSteps = false, .animations = true, .inverted = false, .bold = false };
#define SETTINGS_KEY 1

// Read settings from persistent storage
static void prv_load_settings() {
  // Read settings from persistent storage, if they exist
  persist_read_data(SETTINGS_KEY, &app_config, sizeof(app_config));
  update_time();
}

// Save the settings to persistent storage
static void prv_save_settings() {
  persist_write_data(SETTINGS_KEY, &app_config, sizeof(app_config));
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Longitude
  Tuple *longitude_t = dict_find(iterator, MESSAGE_KEY_KEY_LONGITUDE);
  if (longitude_t) {
    currentlong = TRIG_MAX_ANGLE / 4 + TRIG_MAX_ANGLE * longitude_t->value->int32 / 360;
  }
  // Latidute
  Tuple *latitude_t = dict_find(iterator, MESSAGE_KEY_KEY_LATITUDE);
  if (latitude_t) {
    currentlat = TRIG_MAX_ANGLE / 2 - (TRIG_MAX_ANGLE / 4 - TRIG_MAX_ANGLE  * latitude_t->value->int32 / 360);
  }
  // timezone
  Tuple *timezone_t = dict_find(iterator, MESSAGE_KEY_KEY_TIMEZONE);
  if (timezone_t) {
    timezone_offset = timezone_t->value->int32;
  }

  //   } else if (t->key == MESSAGE_KEY_ShowSteps) {
  //     app_config.showSteps = (t->value->int16 = 1);
  //     APP_LOG(APP_LOG_LEVEL_INFO, "showSteps received %d", (int)t->value->int16);

  // Inverted
  Tuple *inverted_t = dict_find(iterator, MESSAGE_KEY_Inverted);
  if (inverted_t) {
    app_config.inverted = inverted_t->value->int32 == 1;
  }

  // Bold
  Tuple *bold_t = dict_find(iterator, MESSAGE_KEY_Bold);
  if (bold_t) {
    app_config.bold = bold_t->value->int32 == 1;
  }

  // ShowDate
  Tuple *showDate_t = dict_find(iterator, MESSAGE_KEY_ShowDate);
  if (showDate_t) {
    app_config.showDate = showDate_t->value->int32 == 1;
  }

  // Animation
  Tuple *animations_t = dict_find(iterator, MESSAGE_KEY_Animations);
  if (animations_t) {
    app_config.animations = animations_t->value->int32 == 1;
  }

  // Save the new settings to persistent storage
  prv_save_settings();
  update_time();
  layer_mark_dirty(root_layer);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

void message_init(Window *window) {
  root_layer = window_get_root_layer(window);
  prv_load_settings();
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

void message_deinit() {
  app_message_deregister_callbacks();
}
