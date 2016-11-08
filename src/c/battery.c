#include <pebble.h>
#include "battery.h"
#include "message.h"

static Layer *s_battery_layer;
static BatteryChargeState charge_state;

#ifdef PBL_ROUND
static int batteryx = 100;
static int batteryy = 5;
#else
static int batteryx = PBL_DISPLAY_WIDTH - 24;
static int batteryy = 0;
#endif

static void handle_battery(BatteryChargeState state) {
  charge_state = state;
  layer_mark_dirty(s_battery_layer);
}

void battery_layer_update_callback(Layer *layer, GContext *ctx) {
  // Draw the battery:
  if (!app_config.showBattery)
    return;
  #ifdef PBL_BW  
  GColor iconcolor = app_config.inverted ? GColorBlack : GColorWhite;
  #else
  GColor iconcolor = COLOR_FALLBACK((charge_state.charge_percent > 20 ? GColorIslamicGreen : GColorRed) , GColorWhite);
  #endif

  graphics_context_set_stroke_color(ctx, iconcolor);
  graphics_draw_rect(ctx, GRect(6, 4, 14, 8));
  graphics_draw_line(ctx, GPoint(20, 6), GPoint(20, 9));

  charge_state = battery_state_service_peek();
  int width = (charge_state.charge_percent * 10 / 100);
  graphics_context_set_fill_color(ctx, iconcolor);
  graphics_fill_rect(ctx, GRect(8, 6, width, 4), 0, GCornerNone);

  if (charge_state.is_charging) {
    GColor chargecolor = COLOR_FALLBACK(GColorChromeYellow, app_config.inverted ? GColorBlack : GColorWhite);
    graphics_context_set_stroke_color(ctx, chargecolor);
  #ifdef PBL_BW  
    GColor bgcolor = app_config.inverted ? GColorWhite : GColorBlack;
    graphics_context_set_fill_color(ctx, bgcolor);
    graphics_fill_rect(ctx, GRect(10, 4, 6, 8), 0, GCornerNone);
  #endif
    graphics_draw_line(ctx, GPoint(14, 4), GPoint(10, 9));
    graphics_draw_line(ctx, GPoint(15, 4), GPoint(11, 9));
    graphics_draw_line(ctx, GPoint(14, 7), GPoint(10, 12));        
    graphics_draw_line(ctx, GPoint(15, 7), GPoint(11, 12));        
  }
}

void init_battery(Window *window) {
  // Create Battery layer
  s_battery_layer = layer_create(GRect(batteryx, batteryy, 24, 16));
  layer_set_update_proc(s_battery_layer, battery_layer_update_callback);
  layer_add_child(window_get_root_layer(window), s_battery_layer);
}

void destroy_battery() {
  layer_destroy(s_battery_layer);
}
