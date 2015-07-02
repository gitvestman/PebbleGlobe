#include <pebble.h>
#include "message.h"

int currentlong;
int currentlat;
int_fast16_t timezone_offset;

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    // Read first item
  Tuple *t = dict_read_first(iterator);
  
  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
    case KEY_LONGITUDE:
      currentlong = TRIG_MAX_ANGLE / 4 + TRIG_MAX_ANGLE * t->value->int32 / 360;
      break;
    case KEY_LATITUDE:
      currentlat = TRIG_MAX_ANGLE / 2 - (TRIG_MAX_ANGLE / 4 - TRIG_MAX_ANGLE  * t->value->int32 / 360);
      break;
    case KEY_TIMEZONE:
      timezone_offset = t->value->int32;
      APP_LOG(APP_LOG_LEVEL_ERROR, "Timezone offset received %d", (int)timezone_offset);
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }

    // Look for next item
    t = dict_read_next(iterator);
  }
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

void message_init() {
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