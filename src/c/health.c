#include <pebble.h>
#include "health.h"
#include "message.h"

static Layer *s_window_layer;
static TextLayer *s_steps_text_layer;
static TextLayer *s_sleep_text_layer;
static Layer *s_health_layer;
static GFont s_health_font;
static GFont s_health_bold_font;

extern bool animating;

// Create longed-lived buffers
static char stepsbuffer[] = "10.0k";
static char sleepbuffer[] = "13.5h ";

static int get_healt_data(HealthMetric metric) {
    int result = 0;
    #if defined(PBL_HEALTH)
    time_t start = time_start_of_today() - metric == HealthMetricSleepSeconds ? 6 : 0; 
    time_t end = time(NULL);

    // Check the metric has data available for today
    HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric, start, end);

    if(mask & HealthServiceAccessibilityMaskAvailable) {
        // Data is available!
        result = (int)health_service_sum_today(metric);
    }
    #endif
    return result;
}

static int get_healt_average(HealthMetric metric) {
    int result = 0;
    #if defined(PBL_HEALTH)
    // Define query parameters
    const HealthServiceTimeScope scope = HealthServiceTimeScopeWeekly;

    // Use the average daily value from midnight to the current time
    const time_t start = time_start_of_today();
    const time_t end = time(NULL);

    // Check that an averaged value is accessible
    HealthServiceAccessibilityMask mask = 
          health_service_metric_averaged_accessible(metric, start, end, scope);
    if(mask & HealthServiceAccessibilityMaskAvailable) {
        // Average is available, read it
        result = (int)health_service_sum_averaged(metric, start, end, scope);
    }
    #endif
    return result;
}

static void update_health_settings()  {
  // Set fonts
  text_layer_set_font(s_steps_text_layer, app_config.bold ? s_health_bold_font : s_health_font);
  text_layer_set_font(s_sleep_text_layer, app_config.bold ? s_health_bold_font : s_health_font);

  // Set text colors
  GColor background = GColorClear;
  GColor textcolor = app_config.inverted ? GColorBlack : COLOR_FALLBACK(GColorPastelYellow , GColorWhite);

  text_layer_set_background_color(s_steps_text_layer, background);
  text_layer_set_text_color(s_steps_text_layer, textcolor);

  text_layer_set_background_color(s_sleep_text_layer, background);
  text_layer_set_text_color(s_sleep_text_layer, textcolor);
}

static void health_update_proc(Layer *layer, GContext *ctx) {
    graphics_context_set_fill_color(ctx, app_config.inverted ? GColorWhite : GColorBlack);
    graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);

    if (animating || !app_config.showSteps) return;

    update_health_settings(); 
    int steps = get_healt_data(HealthMetricStepCount);
    int sleep = get_healt_data(HealthMetricSleepSeconds);
    int stepsavg = get_healt_average(HealthMetricStepCount);
    int sleepavg = get_healt_average(HealthMetricSleepSeconds);

    snprintf(stepsbuffer, sizeof(stepsbuffer), "%dk", steps/1000);
    text_layer_set_text(s_steps_text_layer, stepsbuffer);

    snprintf(sleepbuffer, sizeof(sleepbuffer), "%dh", sleep/3600);
    text_layer_set_text(s_sleep_text_layer, sleepbuffer);

    GRect bounds = layer_get_bounds(s_window_layer);
    GRect unobstructed_bounds = layer_get_unobstructed_bounds(s_window_layer);
    if (!grect_equal(&bounds, &unobstructed_bounds))
      return; // Don't draw graphs when quickview is enabled

    int maxstepsangle = PBL_IF_ROUND_ELSE(130, 120);
    int minstepsangle = PBL_IF_ROUND_ELSE(80, 70);

    #ifdef PBL_ROUND
    GRect frame = grect_inset(bounds, GEdgeInsets(15, 15, 15, 15));
    #else
    GRect frame = grect_inset(bounds, GEdgeInsets(21, 1, 1, 1));
    #endif

    if (steps > 0 && stepsavg > 0) {
        int y = ((maxstepsangle - minstepsangle) * 3 / 4) * steps / stepsavg;
        if (y > (maxstepsangle - minstepsangle)) y = (maxstepsangle - minstepsangle);

        graphics_context_set_fill_color(ctx, COLOR_FALLBACK(GColorRoseVale, app_config.inverted ? GColorBlack : GColorWhite));
        graphics_fill_radial(ctx, frame, GOvalScaleModeFitCircle, 6, DEG_TO_TRIGANGLE(maxstepsangle - y), DEG_TO_TRIGANGLE(maxstepsangle));        
    }

    if (sleep > 0 && sleepavg > 0) {
        int y = ((maxstepsangle - minstepsangle) * 3 / 4) * sleep / sleepavg;
        if (y > (maxstepsangle - minstepsangle)) y = (maxstepsangle - minstepsangle);

        graphics_context_set_fill_color(ctx, COLOR_FALLBACK(GColorCadetBlue, app_config.inverted ? GColorBlack : GColorWhite));
        graphics_fill_radial(ctx, frame, GOvalScaleModeFitCircle, 6, DEG_TO_TRIGANGLE(- maxstepsangle), DEG_TO_TRIGANGLE( - (maxstepsangle - y)));        
    }
}

void init_health(Window *window) {
  s_window_layer = window_get_root_layer(window);

  #if defined(PBL_HEALTH)
  GRect bounds = layer_get_bounds(s_window_layer);
  int stepsx = bounds.size.w - PBL_IF_ROUND_ELSE(41, 31);
  int sleepx = PBL_IF_ROUND_ELSE(11, 1);
  int stepsy = bounds.size.h/2 - bounds.size.h/4 + PBL_IF_ROUND_ELSE(5, 0);
  int sleepy = stepsy;

  // Create steps textlayer
  s_steps_text_layer = text_layer_create(GRect(stepsx, stepsy, 30, 18));

  // Create sleep textlayer
  s_sleep_text_layer = text_layer_create(GRect(sleepx, sleepy, 30, 18));

  // Create health layer
  s_health_layer = layer_create(bounds);
  layer_set_update_proc(s_health_layer, health_update_proc);
  layer_add_child(window_get_root_layer(window), s_health_layer);

  // Create Fonts
  s_health_font = fonts_get_system_font(FONT_KEY_GOTHIC_18);
  s_health_bold_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);

  text_layer_set_text_alignment(s_steps_text_layer, GTextAlignmentRight);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_steps_text_layer));

  text_layer_set_text_alignment(s_sleep_text_layer, GTextAlignmentLeft);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_sleep_text_layer));

  update_health_settings();
  #endif
}

void destroy_health() {
  text_layer_destroy(s_steps_text_layer);
  text_layer_destroy(s_sleep_text_layer);
  layer_destroy(s_health_layer);
}

