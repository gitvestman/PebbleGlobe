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
#if defined(PBL_PLATFORM_DIORITE) || defined(PBL_PLATFORM_EMERY)    
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
    #if defined(PBL_PLATFORM_DIORITE) || defined(PBL_PLATFORM_EMERY)    
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
    if (!app_config.showHealth) {
        layer_set_hidden((Layer *)s_steps_text_layer, true);
        layer_set_hidden((Layer *)s_sleep_text_layer, true);
    } else {
        layer_set_hidden((Layer *)s_steps_text_layer, false);
        layer_set_hidden((Layer *)s_sleep_text_layer, false);

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

        #if defined(PBL_PLATFORM_DIORITE) || defined(PBL_PLATFORM_EMERY)    
        text_layer_set_font(s_pulse_text_layer, app_config.bold ? s_pulse_bold_font : s_pulse_font);
        text_layer_set_background_color(s_pulse_text_layer, background);
        text_layer_set_text_color(s_pulse_text_layer, textcolor);
        #endif
    }
}

static void health_update_proc(Layer *layer, GContext *ctx) {
    update_health_settings(); 
    if ((animating && !firstframe) || !app_config.showHealth) return;
    int steps = get_health_data(HealthMetricStepCount);
    int sleep = get_health_data(HealthMetricSleepSeconds);
    int stepsavg = get_health_average(HealthMetricStepCount, true);
    int sleepavg = get_health_average(HealthMetricSleepSeconds, true);
    #if defined(PBL_PLATFORM_DIORITE) || defined(PBL_PLATFORM_EMERY)    
    uint32_t pulse = health_service_peek_current_value(HealthMetricHeartRateBPM);
    //pulse = 100;
    snprintf(pulsebuffer, sizeof(pulsebuffer), "%ld❤️", pulse);
    text_layer_set_text(s_pulse_text_layer, pulsebuffer);
    #endif

    //steps = 22000;
    //stepsavg = 4000;
    //sleep = 24000;
    //sleepavg = 24000;

    snprintf(stepsbuffer, sizeof(stepsbuffer), "%dk", steps/1000);
    text_layer_set_text(s_steps_text_layer, stepsbuffer);

    snprintf(sleepbuffer, sizeof(sleepbuffer), "%dh", sleep/3600);
    text_layer_set_text(s_sleep_text_layer, sleepbuffer);

    GRect bounds = layer_get_bounds(s_window_layer);
    GRect unobstructed_bounds = layer_get_unobstructed_bounds(s_window_layer);
    if ((animating && !firstframe) || !grect_equal(&bounds, &unobstructed_bounds))
      return; // Don't draw graphs when quickview is enabled or animating

    #ifdef PBL_ROUND
    int maxstepsangle = PBL_IF_ROUND_ELSE(140, 120);
    int minstepsangle = PBL_IF_ROUND_ELSE(90, 70);

    GRect frame = grect_inset(bounds, GEdgeInsets(15, 15, 15, 15));
    GRect frame2 = grect_inset(bounds, GEdgeInsets(13, 13, 13, 13));
    #endif 

    #ifdef PBL_PLATFORM_EMERY
    int bladewidth = 10;
    int bladestart = 190;
    int bladelength = 100;
    int blademargin = 7;
    #else
    int bladewidth = 8;
    int bladestart = 135;
    int bladelength = 70;
    int blademargin = 5;
    #endif

    if (steps > 0 && stepsavg > 0) {
        GColor strokecolor = COLOR_FALLBACK(app_config.inverted ? GColorMelon : GColorRed, GColorLightGray);
        GColor fillcolor = COLOR_FALLBACK(app_config.inverted ? GColorRed : GColorMelon, app_config.inverted ? GColorBlack : GColorWhite);
        #ifdef PBL_ROUND
        int y = ((maxstepsangle - minstepsangle) * 3 / 4) * steps / stepsavg;
        if (y > (maxstepsangle - minstepsangle)) y = (maxstepsangle - minstepsangle);
        graphics_context_set_fill_color(ctx, strokecolor);
        graphics_fill_radial(ctx, frame2, GOvalScaleModeFitCircle, 8, DEG_TO_TRIGANGLE(maxstepsangle - y), DEG_TO_TRIGANGLE(maxstepsangle));
        graphics_context_set_fill_color(ctx, fillcolor);
        graphics_fill_radial(ctx, frame, GOvalScaleModeFitCircle, 4, DEG_TO_TRIGANGLE(maxstepsangle - y), DEG_TO_TRIGANGLE(maxstepsangle));
        #else
        int y = (bladelength * 3 / 4) * steps / stepsavg;
        if (y > bladelength) y = bladelength;
        graphics_context_set_fill_color(ctx, strokecolor);
        GRect rect_outer_bounds = GRect(bounds.size.w - bladewidth - blademargin, bladestart - y, bladewidth, y);
        GRect rect_inner_bounds = GRect(bounds.size.w - bladewidth - blademargin + 2, bladestart - y + 2, bladewidth - 4, y - 4);
        graphics_fill_rect(ctx, rect_outer_bounds, 4, GCornersAll);              
        graphics_context_set_fill_color(ctx, fillcolor);
        graphics_fill_rect(ctx, rect_inner_bounds, 2, GCornersAll);              
        GRect rect_bounds = GRect(bounds.size.w - bladewidth - blademargin + 2, bladestart -2, bladewidth - 4, bladewidth + 2);
        graphics_context_set_fill_color(ctx, GColorLightGray);
        graphics_fill_rect(ctx, rect_bounds, 0, GCornersAll);      
        #endif
    }

    if (sleep > 0 && sleepavg > 0) {
        GColor strokecolor = COLOR_FALLBACK(app_config.inverted ? GColorBabyBlueEyes: GColorBlue, GColorLightGray);
        GColor fillcolor = COLOR_FALLBACK(app_config.inverted ? GColorBlue : GColorBabyBlueEyes, app_config.inverted ? GColorBlack : GColorWhite);
        #ifdef PBL_ROUND
        int y = ((maxstepsangle - minstepsangle) * 3 / 4) * sleep / sleepavg;
        if (y > (maxstepsangle - minstepsangle)) y = (maxstepsangle - minstepsangle);
        graphics_context_set_fill_color(ctx, strokecolor);
        graphics_fill_radial(ctx, frame2, GOvalScaleModeFitCircle, 8, DEG_TO_TRIGANGLE(- maxstepsangle), DEG_TO_TRIGANGLE( - (maxstepsangle - y)));
        graphics_context_set_fill_color(ctx, fillcolor);
        graphics_fill_radial(ctx, frame, GOvalScaleModeFitCircle, 4, DEG_TO_TRIGANGLE(- maxstepsangle), DEG_TO_TRIGANGLE( - (maxstepsangle - y)));
        #else
        int y = (bladelength * 4 / 5) * sleep / sleepavg;
        if (y > bladelength) y = bladelength;
        GRect rect_outer_bounds = GRect(blademargin, bladestart - y, bladewidth, y);
        GRect rect_inner_bounds = GRect(blademargin + 2, bladestart - y + 2, bladewidth - 4, y - 4);
        graphics_context_set_fill_color(ctx, strokecolor);
        graphics_fill_rect(ctx, rect_outer_bounds, 4, GCornersAll);      
        graphics_context_set_fill_color(ctx, fillcolor);
        graphics_fill_rect(ctx, rect_inner_bounds, 2, GCornersAll);      
        GRect rect_bounds = GRect(blademargin + 2, bladestart -2, bladewidth - 4, bladewidth + 2);
        graphics_context_set_fill_color(ctx, GColorLightGray);
        graphics_fill_rect(ctx, rect_bounds, 0, GCornersAll);      
        #endif        
    }
}

void init_health(Window *window) {
  s_window_layer = window_get_root_layer(window);

  GRect bounds = layer_get_bounds(s_window_layer);
  int stepsx = bounds.size.w - PBL_IF_ROUND_ELSE(41, 31);
  int sleepx = PBL_IF_ROUND_ELSE(11, 1);
  int stepsy = bounds.size.h/2 - bounds.size.h/4 + PBL_IF_ROUND_ELSE(20, 0);
  int sleepy = stepsy;

  // Create steps textlayer
  s_steps_text_layer = text_layer_create(GRect(stepsx, stepsy, 30, 24));

  // Create sleep textlayer
  s_sleep_text_layer = text_layer_create(GRect(sleepx, sleepy, 30, 24));

  #if defined(PBL_PLATFORM_DIORITE) || defined(PBL_PLATFORM_EMERY)    
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
  #ifdef PBL_PLATFORM_EMERY
  s_health_font = fonts_get_system_font(FONT_KEY_GOTHIC_24);
  s_health_bold_font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  #else
  s_health_font = fonts_get_system_font(FONT_KEY_GOTHIC_18);
  s_health_bold_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  #endif

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

