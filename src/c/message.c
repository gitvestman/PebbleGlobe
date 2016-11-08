#include <pebble.h>
#include "message.h"
#include "clock.h"

Window* window_ref;
Layer* root_layer;
int currentlong;
int currentlat;
int16_t timezone_offset;
Config app_config = { .showDate = true, .animations = true, .inverted = false, .bold = false, .showBattery = true };
GColor background_color;
#define SETTINGS_KEY 1

// Read settings from persistent storage
static void prv_load_settings() {
  // Read settings from persistent storage, if they exist
  persist_read_data(SETTINGS_KEY, &app_config, sizeof(app_config));
  background_color = app_config.inverted ? GColorWhite : GColorBlack;
  update_time();
  layer_mark_dirty(root_layer);
}

// Save the settings to persistent storage
static void prv_save_settings() {
  persist_write_data(SETTINGS_KEY, &app_config, sizeof(app_config));
  window_set_background_color(window_ref, app_config.inverted ? GColorWhite : GColorBlack);
  background_color = app_config.inverted ? GColorWhite : GColorBlack;
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
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

  // showBattery
  Tuple *showBattery_t = dict_find(iterator, MESSAGE_KEY_ShowBattery);
  if (showBattery_t) {
    app_config.showBattery = showBattery_t->value->int32 == 1;
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

void root_layer_update_proc(Layer *layer, GContext *ctx)
{
  if (background_color.argb != GColorClearARGB8) 
  {
    graphics_context_set_fill_color(ctx, background_color);
    graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
  }
}

void message_init(Window *window) {
  window_ref = window;
  root_layer = window_get_root_layer(window);
  layer_set_update_proc(root_layer, root_layer_update_proc);
  prv_load_settings();
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage
  app_message_open(320, 0);
}

void message_deinit() {
  app_message_deregister_callbacks();
}
