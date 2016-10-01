#include <pebble.h>
#include "health.h"
#include "message.h"

#ifdef PBL_HEALTH
static Layer *s_window_layer;

static TextLayer *s_steps_text_layer;
static TextLayer *s_sleep_text_layer;
static Layer *s_health_layer;
static GFont s_health_font;
static GFont s_health_bold_font;
#ifdef PBL_PLATFORM_DIORITE
static TextLayer *s_pulse_text_layer;
static GFont s_pulse_font;
static GFont s_pulse_bold_font;
static char pulsebuffer[] = "200❤️";
#endif


extern bool animating;
extern bool firstframe;

// Create longed-lived buffers
static char stepsbuffer[] = "10.0k";
static char sleepbuffer[] = "13.5h ";

void health_unobstructed_did_change(void *context) {
    #ifdef PBL_PLATFORM_DIORITE
    // Get the total available screen real-estate
    GRect unobstructed_bounds = layer_get_unobstructed_bounds(s_window_layer);
    // Move pulse layer  
    layer_set_frame((Layer *)s_pulse_text_layer, GRect(1, unobstructed_bounds.size.h - 30, 100, 30));
    #endif
}

static int get_health_data(HealthMetric metric) {
    int result = 0;
    const time_t start = time_start_of_today(); 
    const time_t end = time(NULL);

    // Check the metric has data available for today
    HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric, start, end);

    if(mask & HealthServiceAccessibilityMaskAvailable) {
        // Data is available!
        result = (int)health_service_sum_today(metric);
    }
    return result;
}

static HealthValue get_health_average(HealthMetric metric, bool daily) {
    HealthValue result = 0;
    // Define query parameters
    const HealthServiceTimeScope scope = HealthServiceTimeScopeWeekly;

    // Use the average daily value from midnight to the current time
    time_t start = time_start_of_today();
    const time_t end = time(NULL);
    if (!daily) {
        start = end - 10; // Last 10 seconds
    }

    // Check that an averaged value is accessible
    HealthServiceAccessibilityMask mask = 
          health_service_metric_averaged_accessible(metric, start, end, scope);
    if(mask & HealthServiceAccessibilityMaskAvailable) {
        // Average is available, read it
        result = health_service_sum_averaged(metric, start, end, scope);
    }
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

    #ifdef PBL_PLATFORM_DIORITE
    text_layer_set_font(s_pulse_text_layer, app_config.bold ? s_pulse_bold_font : s_pulse_font);
    text_layer_set_background_color(s_pulse_text_layer, background);
    text_layer_set_text_color(s_pulse_text_layer, textcolor);
    #endif
}

static void health_update_proc(Layer *layer, GContext *ctx) {
    update_health_settings(); 
    if ((animating && !firstframe) || !app_config.showHealth) return;
    int steps = get_health_data(HealthMetricStepCount);
    int sleep = get_health_data(HealthMetricSleepSeconds);
    int stepsavg = get_health_average(HealthMetricStepCount, true);
    int sleepavg = get_health_average(HealthMetricSleepSeconds, true);
    #ifdef PBL_PLATFORM_DIORITE    
    int pulseavg = get_health_average(HealthMetricHeartRateBPM, false);
    pulseavg = 100;
    snprintf(pulsebuffer, sizeof(pulsebuffer), "%d❤️", pulseavg);
    text_layer_set_text(s_pulse_text_layer, pulsebuffer);
    #endif

    steps = 6000;
    stepsavg = 4000;
    sleep = 24000;
    sleepavg = 24000;


    snprintf(stepsbuffer, sizeof(stepsbuffer), "%dk", steps/1000);
    text_layer_set_text(s_steps_text_layer, stepsbuffer);

    snprintf(sleepbuffer, sizeof(sleepbuffer), "%dh", sleep/3600);
    text_layer_set_text(s_sleep_text_layer, sleepbuffer);

    GRect bounds = layer_get_bounds(s_window_layer);
    GRect unobstructed_bounds = layer_get_unobstructed_bounds(s_window_layer);
    if ((animating && !firstframe) || !grect_equal(&bounds, &unobstructed_bounds))
      return; // Don't draw graphs when quickview is enabled or animating

    int maxstepsangle = PBL_IF_ROUND_ELSE(140, 120);
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

  GRect bounds = layer_get_bounds(s_window_layer);
  int stepsx = bounds.size.w - PBL_IF_ROUND_ELSE(41, 31);
  int sleepx = PBL_IF_ROUND_ELSE(11, 1);
  int stepsy = bounds.size.h/2 - bounds.size.h/4 + PBL_IF_ROUND_ELSE(5, 0);
  int sleepy = stepsy;

  // Create steps textlayer
  s_steps_text_layer = text_layer_create(GRect(stepsx, stepsy, 30, 18));

  // Create sleep textlayer
  s_sleep_text_layer = text_layer_create(GRect(sleepx, sleepy, 30, 18));

  #ifdef PBL_PLATFORM_DIORITE
  // Create pulse textlayer
  GRect unobstructed_bounds = layer_get_unobstructed_bounds(s_window_layer);
  s_pulse_text_layer = text_layer_create(GRect(sleepx, unobstructed_bounds.size.h - 30, 60, 30));
  // Create Fonts
  s_pulse_font = fonts_get_system_font(FONT_KEY_GOTHIC_28);
  s_pulse_bold_font = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  // Align
  text_layer_set_text_alignment(s_steps_text_layer, GTextAlignmentLeft);
  // Add to windows
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_pulse_text_layer));
  #endif

  // Create health graph layer
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
}

void destroy_health() {
  #ifdef PBL_PLATFORM_DIORITE    
  text_layer_destroy(s_pulse_text_layer);
  #endif
  text_layer_destroy(s_steps_text_layer);
  text_layer_destroy(s_sleep_text_layer);
  layer_destroy(s_health_layer);
}
#endif

