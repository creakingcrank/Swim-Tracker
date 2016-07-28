#include <pebble.h>
#include "main_window.h"
#include "length_data.h"
#include "interval_data.h"
#include "comms.h"
#include "pool_data.h"
#include "detect.h"

#define DEBUG  /* debugging code */

#define ACCEL_STEP_MS 20         /* frequency of accelerometer checks */


static bool paused = true;  // toggled by set button

/*

THINGS TO DO

-- Comms/Appmessage
--Keep main clear

 */

// BEGIN AUTO-GENERATED UI CODE; DO NOT MODIFY
static Window *s_window;
static GFont s_res_bitham_42_bold;
static GFont s_res_gothic_28;
static GFont s_res_bitham_30_black;
static GFont s_res_gothic_28_bold;
static TextLayer *lengths_layer;
static TextLayer *strokes_layer;
static TextLayer *stopwatch_layer;
static TextLayer *l_text;
static TextLayer *pool_layer;
static TextLayer *intervals_layer;

static void initialise_ui(void) {
  s_window = window_create();
  #ifndef PBL_SDK_3
    window_set_fullscreen(s_window, true);
  #endif
  
  s_res_bitham_42_bold = fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD);
  s_res_gothic_28 = fonts_get_system_font(FONT_KEY_GOTHIC_28);
  s_res_bitham_30_black = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);
  s_res_gothic_28_bold = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  // lengths_layer
  lengths_layer = text_layer_create(GRect(0, 46, 145, 42));
  text_layer_set_background_color(lengths_layer, GColorClear);
  text_layer_set_text(lengths_layer, "0");
  text_layer_set_text_alignment(lengths_layer, GTextAlignmentCenter);
  text_layer_set_font(lengths_layer, s_res_bitham_42_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)lengths_layer);
  
  // strokes_layer
  strokes_layer = text_layer_create(GRect(100, 130, 43, 30));
  text_layer_set_background_color(strokes_layer, GColorClear);
  text_layer_set_text(strokes_layer, "---");
  text_layer_set_text_alignment(strokes_layer, GTextAlignmentRight);
  text_layer_set_font(strokes_layer, s_res_gothic_28);
  layer_add_child(window_get_root_layer(s_window), (Layer *)strokes_layer);
  
  // stopwatch_layer
  stopwatch_layer = text_layer_create(GRect(0, 95, 145, 35));
  text_layer_set_background_color(stopwatch_layer, GColorBlack);
  text_layer_set_text_color(stopwatch_layer, GColorWhite);
  text_layer_set_text(stopwatch_layer, "0:00:00");
  text_layer_set_text_alignment(stopwatch_layer, GTextAlignmentCenter);
  text_layer_set_font(stopwatch_layer, s_res_bitham_30_black);
  layer_add_child(window_get_root_layer(s_window), (Layer *)stopwatch_layer);
  
  // l_text
  l_text = text_layer_create(GRect(0, 11, 145, 34));
  text_layer_set_background_color(l_text, GColorClear);
  text_layer_set_text(l_text, "Lengths");
  text_layer_set_text_alignment(l_text, GTextAlignmentCenter);
  text_layer_set_font(l_text, s_res_bitham_30_black);
  layer_add_child(window_get_root_layer(s_window), (Layer *)l_text);
  
  // pool_layer
  pool_layer = text_layer_create(GRect(0, 130, 52, 30));
  text_layer_set_background_color(pool_layer, GColorClear);
  text_layer_set_text(pool_layer, "25m");
  text_layer_set_font(pool_layer, s_res_gothic_28_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)pool_layer);
  
  // intervals_layer
  intervals_layer = text_layer_create(GRect(51, 130, 45, 30));
  text_layer_set_background_color(intervals_layer, GColorClear);
  text_layer_set_text(intervals_layer, "w");
  text_layer_set_text_alignment(intervals_layer, GTextAlignmentCenter);
  text_layer_set_font(intervals_layer, s_res_gothic_28_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)intervals_layer);
}

static void destroy_ui(void) {
  window_destroy(s_window);
  text_layer_destroy(lengths_layer);
  text_layer_destroy(strokes_layer);
  text_layer_destroy(stopwatch_layer);
  text_layer_destroy(l_text);
  text_layer_destroy(pool_layer);
  text_layer_destroy(intervals_layer);
}
// END AUTO-GENERATED UI CODE

double square(double num) {
  return num * num;
}

int get_sqrt( int num ) {
  // Uses Babylonian sequence to approximate root of num - this borrowed from pebble accel discs example, but converted to integer
  int approx, product, b_seq;
  int tolerance = 5; //good enough for me, since I'm working with numbers around the 1000 mark

  approx = num;
  product = square(approx);
  // Check if the square of our approximation is within the error tolerance of num
  while(product - num >= tolerance) {
    b_seq = ( approx + ( num / approx ) ) / 2;
    approx = b_seq;
    product = square(approx);
  }
  return approx;
}



static void update_elapsed_time_display(int main_display_setting){
  
  static char time_to_display[9];
  
  int workout_elapsed_time = get_total_interval_duration(1, get_current_interval());
  int interval_elapsed_time = get_interval_duration(get_current_interval());
  
  
  if (!paused){
    text_layer_set_background_color(stopwatch_layer, GColorWhite);
    text_layer_set_text_color(stopwatch_layer, GColorBlack);
  }
  else {
   text_layer_set_background_color(stopwatch_layer, GColorBlack);
   text_layer_set_text_color(stopwatch_layer, GColorWhite);
  }
  
  // if you are watching overall data, show workout time, else show interval time
  if (main_display_setting < 3) snprintf(time_to_display,sizeof(time_to_display),"%01d:%02d:%02d",workout_elapsed_time / 3600, (workout_elapsed_time / 60) % 60, workout_elapsed_time  % 60 );
  else snprintf(time_to_display,sizeof(time_to_display),"%01d:%02d:%02d",interval_elapsed_time / 3600, (interval_elapsed_time / 60) % 60, interval_elapsed_time  % 60 );
  text_layer_set_text(stopwatch_layer, time_to_display);
  
}

 int update_main_display(int setting_change) {
  
  static char lengths_text_to_display [8];
   
  static int main_display_setting = 0; 
  int pace;
   
  main_display_setting = main_display_setting+setting_change;
  if (main_display_setting > 6) main_display_setting = 0;
  if (main_display_setting < 0) main_display_setting = 6;
   
 
    switch (main_display_setting) {
    case 0 :
      snprintf(lengths_text_to_display,sizeof(lengths_text_to_display),"%d", get_total_number_of_lengths());
      break;
    case 1 :
      snprintf(lengths_text_to_display,sizeof(lengths_text_to_display),"%dm", get_total_number_of_lengths()*pool_length(0));
      break;
    case 2 :
      pace = get_workout_pace() * 100 / pool_length(0);
      snprintf(lengths_text_to_display,sizeof(lengths_text_to_display),"%01d:%02d", (pace / 60) % 60, pace % 60 );
      break;
    case 3 :
      snprintf(lengths_text_to_display,sizeof(lengths_text_to_display),"%d", get_interval_lengths(get_current_interval()));
      break;
    case 4 :
       snprintf(lengths_text_to_display,sizeof(lengths_text_to_display),"%dm", get_interval_lengths(get_current_interval())*pool_length(0));
      break;
    case 5 :
      pace = get_interval_pace(get_current_interval()) * 100 / pool_length(0);
      snprintf(lengths_text_to_display,sizeof(lengths_text_to_display),"%01d:%02d", (pace / 60) % 60, pace % 60 );
      break;
    case 6 :
      snprintf(lengths_text_to_display,sizeof(lengths_text_to_display),"%d", get_interval_stroke_rate(get_current_interval()));
      break;
 
  }   
 
// Display that text:  
   
text_layer_set_text(lengths_layer,lengths_text_to_display);
   
 update_elapsed_time_display(main_display_setting);   
   
 return main_display_setting;   
}

void update_status_display(int main_display_setting) {
  
  static char intervals_text[9];
  
  switch (main_display_setting) {
    case 0 :
      text_layer_set_text(l_text, "Lengths");
      break;
    case 1 :
      text_layer_set_text(l_text, "Distance");
      break;
    case 2 :
      text_layer_set_text(l_text, "Pace");
      break;
    case 3 :
      text_layer_set_text(l_text, "Lengths");
      break;
    case 4 :
      text_layer_set_text(l_text, "Distance");
      break;
    case 5 :
      text_layer_set_text(l_text, "Pace");
      break;
    case 6 :
      text_layer_set_text(l_text, "str/min");
      break;
  }
  
  switch (main_display_setting) {
    case 0 :
    case 1 :
    case 2 :
      snprintf(intervals_text,sizeof(intervals_text),"w");
      break;
    case 3 :
    case 4 : 
    case 5 :
    case 6 :
     snprintf(intervals_text,sizeof(intervals_text),"i%d",get_current_interval());
    break; 
  }
  text_layer_set_text(intervals_layer,intervals_text);
  
}  

static void update_pool_display(int change) {
  
  static char pool_text_to_display [5]; 
  
  snprintf(pool_text_to_display,sizeof(pool_text_to_display),"%dm", pool_length(change));
  text_layer_set_text(pool_layer,pool_text_to_display);
  
  update_main_display(0);
  
  
}

static void timer_callback(void *data) { // main loop to collect acceleration data
 

int total_acc;
time_t t_s;
uint16_t t_ms;
int current_time;  
AccelData accel = (AccelData) { .x = 0, .y = 0, .z = 0, .did_vibrate = false, .timestamp = 0 };
  
  if (paused) return; // end here if not swimming 

accel_service_peek(&accel); //get accelerometer data

time_ms(&t_s, &t_ms); 
current_time = t_s*1000 + t_ms; //get current time in ms
  
if (!accel.did_vibrate) { //ignore readings polluted by vibes (1st few always are as start sets off vibration)
  
  total_acc = get_sqrt(square(accel.x)+square(accel.y)+square(accel.z)); //calculate overall acc using pythagoras
  if (count_strokes(total_acc, current_time)) update_status_display(update_main_display(0));  // call the stroke count function and update display if required

}
  
app_timer_register(ACCEL_STEP_MS, timer_callback, NULL); 
}

void hide_main_window(void) {
  window_stack_remove(s_window, true);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) { //timer on/off
  
  time_t current_time = time(NULL);
  
  paused = !paused;
  vibes_double_pulse();
  #ifdef DEBUG
    APP_LOG(APP_LOG_LEVEL_INFO,"Timer triggered at %ds",get_workout_duration());
  #endif
  if (!paused) { 
    if (get_total_number_of_lengths() <1 ) {
      set_length( 1, current_time, current_time, 0); // put a valid initial time in so the clock makes sense until 1st length recorded.
    }
    timer_callback(0);
  }
update_main_display(0);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) { //toggle main display
  

 update_status_display(update_main_display(1)); 

}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) { //toggle pool length
  
  update_pool_display(1);
}

static void long_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  
  
  // RESET!!!!! 
  if (paused) {
    set_current_length(1);
    set_length(1,0,0,0); // clear data in current length - leave program to overwrite other data
    set_current_interval(1);
    set_interval(1,1,1);
    vibes_long_pulse();
    update_elapsed_time_display(update_main_display(0));
  }
}

static void long_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  dump_data_to_app_log();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 3000, long_select_click_handler ,NULL );
   window_long_click_subscribe(BUTTON_ID_DOWN, 3000,long_down_click_handler, NULL );
}
static void handle_window_unload(Window* window) {
  destroy_ui();
}

void show_main_window(void) {
  initialise_ui();
  window_set_click_config_provider(s_window, click_config_provider);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
window_stack_push(s_window, true);
}

static void init() {
  read_data_from_persist();
  show_main_window();
  update_main_display(0);
  update_pool_display(0);
  
}

static void deinit() {
  dump_data_to_persist();
  dump_data_to_app_log();

}


int main(void) {
  
  init();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);
  app_event_loop();
  deinit();
  
}