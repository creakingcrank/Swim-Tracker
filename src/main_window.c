#include <pebble.h>
#include "main_window.h"
#include "length_data.h"
#include "interval_data.h"
#include "comms.h"

 #define DEBUG  /* debugging code */

#define ACCEL_STEP_MS 20         /* frequency of accelerometer checks */
#define ACC_THRESHOLD 1100       /* threshold for an acceleration peak - stationary wrist is about 1000 */
#define STROKE_MIN_PEAK_TIME_MS 150     /* minimim duration of peak to count it */
#define MIN_STROKES_PER_LENGTH 5 /* minimum number of recorded strokes for a length to be valid, current used in a display function only */ 
#define TRIGGER_AUTO_INTERVAL_AFTER_S 10 /* number of seconds to wait before auto interval trigger */
#define AVE_STROKES_PER_LENGTH_FLOOR 12 /* A seed number for average strokes per length, used for 1st length in interval only, after that, we use real ave */
#define INITIAL_AVERAGE_PEAK_TO_PEAK_TIME_MS 1000 /* a seed number for the avergae time between peaks, used for length end sensing, adapted during swimming */

// define constants for missing stroke detection
#define MAX_AVE_PEAK_TO_PEAK_TIME_MS 2500 // the initial, and maximum allowed, average time between peaks
#define MISSING_PEAK_SENS 9/4 // The number of average peak gaps we wait before a length check - integer calculation so use fractions here


static int ave_strokes_per_length = AVE_STROKES_PER_LENGTH_FLOOR; // Avergae number of strokes per length, learned during swim

static bool paused = true;  // toggled by set button

static int pool_length[] = {25, 33, 50};
static int number_of_pool_lengths = sizeof(pool_length)/sizeof(pool_length[0]);
static int current_pool_length = 0; //pool lengths toggle by down click

static int main_display_setting = 0; // what we show on the main screen 
                                    // 0 = number of lengths, 1 = distance covered, 2 = average pace /100m, 
                                    // 3 =last length strole rate /min  
                                    // toggle by up click

/*

THINGS TO DO

-- Get rid of remainign global variables
-- Further modularization: pull out stroke/length algorithm

 */

// BEGIN AUTO-GENERATED UI CODE; DO NOT MODIFY
static Window *s_window;
static GFont s_res_bitham_42_bold;
static GFont s_res_gothic_28_bold;
static GFont s_res_bitham_30_black;
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
  s_res_gothic_28_bold = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  s_res_bitham_30_black = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);
  // lengths_layer
  lengths_layer = text_layer_create(GRect(0, 46, 145, 42));
  text_layer_set_text(lengths_layer, "0");
  text_layer_set_text_alignment(lengths_layer, GTextAlignmentCenter);
  text_layer_set_font(lengths_layer, s_res_bitham_42_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)lengths_layer);
  
  // strokes_layer
  strokes_layer = text_layer_create(GRect(90, 130, 53, 30));
  text_layer_set_text(strokes_layer, "0 str");
  text_layer_set_text_alignment(strokes_layer, GTextAlignmentRight);
  text_layer_set_font(strokes_layer, s_res_gothic_28_bold);
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
  text_layer_set_text(l_text, "Lengths");
  text_layer_set_text_alignment(l_text, GTextAlignmentCenter);
  text_layer_set_font(l_text, s_res_bitham_30_black);
  layer_add_child(window_get_root_layer(s_window), (Layer *)l_text);
  
  // pool_layer
  pool_layer = text_layer_create(GRect(0, 130, 40, 30));
  text_layer_set_text(pool_layer, "25m");
  text_layer_set_font(pool_layer, s_res_gothic_28_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)pool_layer);
  
  // intervals_layer
  intervals_layer = text_layer_create(GRect(45, 130, 45, 30));
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

static bool length_end_check(int strokes) {
  
  bool return_value;
  
  if (strokes >= ave_strokes_per_length/4) { // If we've done more than 25 percent of the average strokes, increment the length
      ave_strokes_per_length = ((ave_strokes_per_length * (get_interval_lengths(get_current_interval())-1)) + strokes) / get_interval_lengths(get_current_interval()); // update average strokes per length
      vibes_long_pulse(); // for testing only
      return_value = true;
     
  }
  else { // else assume false alarm and reset the length
    return_value = false;
    #ifdef DEBUG 
        APP_LOG(APP_LOG_LEVEL_INFO, "Insufficient strokes, restarting length");
      #endif
  }
    
    if (ave_strokes_per_length < AVE_STROKES_PER_LENGTH_FLOOR) ave_strokes_per_length = AVE_STROKES_PER_LENGTH_FLOOR; // Keep ave strokes per length sensible
    
return return_value;
}


static int get_missing_peak_window(int ap2p, int astrokes, int stroke) {
  
  /* 
  function to return a time window for length end detection.
  Window gets smaller as length progresses
  No peak detected in the window = a length turn
  */
  
  int base_sense_percent = 200; // this is the minimum ap2p multiplier, should never be less than 200
  int variable_sense_percent = 200; // this is the varialbe element, falls through the length
  int missing_peak_window;
  
  int percent_of_length_left = 0;
  
  
  if (stroke < astrokes) percent_of_length_left = 100*(astrokes - stroke)/astrokes;
  
  missing_peak_window =  ap2p * (base_sense_percent + (variable_sense_percent*percent_of_length_left/100)) / 100;
  
  return missing_peak_window;
  
}

static void count_strokes(int accel, int timestamp) {
  
  /*
  
  Here's the algorithm: 
  
  A stroke has two acceleration peaks in quick succession (up and down or out and back) (strokes = peaks /2)
  We count a peak if accel stays above ACC_THRESHOLD for longer than MIN_PEAK_TIME
  For each length, we track the average time between peaks.
  If we don't see another peak within missing_peak_window we call the increment lengths function
  
  
  */
 static int strokes = 0;     //counter for strokes recognised in current length
 static int peaks = 0;       //counter for acceleration peaks recognised, 2 peaks makes a stroke 
 static int latest_peak_to_peak_time_ms; // The time between the end of detected peaks
 static int ave_peak_to_peak_time_ms = INITIAL_AVERAGE_PEAK_TO_PEAK_TIME_MS; // Average time between acceleration peaks, learned during swim  
 static int up_time = 0;     // time current peak started, to discount short peaks from measurement
 static int last_peak_time = 0; // measure overall stroke time from beginning of 1st peak
 static bool up = false;     // have we logged an acceleration peak?
 static time_t start_of_length_time; // The time the first acceleration peak in a length is recorded 
  
 static char strokes_text_to_display [12]; 
   

  
  if ((accel > ACC_THRESHOLD) && (up == false)) { //identify the start of an acc peak
      up = true;
      up_time = timestamp;
    }
  
  if ( (accel < ACC_THRESHOLD) && (up == true) )  { //identify the end of an acc peak
    up = false;
    
    if (timestamp-up_time > STROKE_MIN_PEAK_TIME_MS) { // was it long enough to count as a peak?
      
      latest_peak_to_peak_time_ms = timestamp - last_peak_time;
      
      if (strokes >= ave_strokes_per_length/4) { // this used to be if (peaks > 4) - so that movement between lengths doesnt affect ap2p
        
        ave_peak_to_peak_time_ms = ((ave_peak_to_peak_time_ms * (peaks-1)) + latest_peak_to_peak_time_ms  )/ peaks; 
        
        if (ave_peak_to_peak_time_ms > MAX_AVE_PEAK_TO_PEAK_TIME_MS) ave_peak_to_peak_time_ms = MAX_AVE_PEAK_TO_PEAK_TIME_MS; // if avep2p gets too big, cap it.
        
        snprintf(strokes_text_to_display,sizeof(strokes_text_to_display),"%d", ave_peak_to_peak_time_ms); // this for testing...
        text_layer_set_text(strokes_layer,strokes_text_to_display);
      }
      
      last_peak_time = timestamp;
      peaks++; //if the peak was long enough, log it
      strokes = peaks / 2;
      if (peaks == 1) start_of_length_time=time(NULL);
      #ifdef DEBUG
        APP_LOG(APP_LOG_LEVEL_INFO, "Peak %d %dms recorded at %ds, %d strokes, p2p %dms, ap2p %dms, window %dms", peaks, 
                timestamp-up_time, elapsed_time_in_workout(), strokes, latest_peak_to_peak_time_ms, ave_peak_to_peak_time_ms, 
                get_missing_peak_window(ave_peak_to_peak_time_ms, ave_strokes_per_length, strokes));
      #endif
    }
  }    
 

   
  // check for end of length
  
  if ( ((timestamp - last_peak_time) > get_missing_peak_window(ave_peak_to_peak_time_ms, ave_strokes_per_length, strokes)) && (peaks > 0) ) {
    #ifdef DEBUG
      APP_LOG(APP_LOG_LEVEL_INFO, "Peak missed, triggering length check at %ds", elapsed_time_in_workout());
    #endif
    if (length_end_check(strokes)) {
      set_length( 0, start_of_length_time, time(NULL), strokes);
    }
    up = false;
    strokes = 0;
    peaks = 0;
    
  }

  update_main_display();
  update_status_display();



}

static void update_elapsed_time_display(){
  
  static char time_to_display[9];
  
  int workout_elapsed_time = get_total_interval_duration(1, get_current_interval());
  int interval_elapsed_time = get_interval_duration(get_current_interval());
  
  
  if (!paused){
    text_layer_set_background_color(stopwatch_layer, GColorWhite);
    text_layer_set_text_color(stopwatch_layer, GColorBlack);
    app_timer_register(1000, update_elapsed_time_display, NULL );
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

 void update_main_display(void) {
  
  static char lengths_text_to_display [8];
  
  int pace;
 
    switch (main_display_setting) {
    case 0 :
      snprintf(lengths_text_to_display,sizeof(lengths_text_to_display),"%d", get_total_number_of_lengths());
      break;
    case 1 :
      snprintf(lengths_text_to_display,sizeof(lengths_text_to_display),"%dm", get_total_number_of_lengths()*pool_length[current_pool_length]);
      break;
    case 2 :
      pace = get_workout_pace() * 100 / pool_length[current_pool_length];
      snprintf(lengths_text_to_display,sizeof(lengths_text_to_display),"%01d:%02d", (pace / 60) % 60, pace % 60 );
      break;
    case 3 :
      snprintf(lengths_text_to_display,sizeof(lengths_text_to_display),"%d", get_interval_lengths(get_current_interval()));
      break;
    case 4 :
       snprintf(lengths_text_to_display,sizeof(lengths_text_to_display),"%dm", get_interval_lengths(get_current_interval())*pool_length[current_pool_length]);
      break;
    case 5 :
      pace = get_interval_pace(get_current_interval()) * 100 / pool_length[current_pool_length];
      snprintf(lengths_text_to_display,sizeof(lengths_text_to_display),"%01d:%02d", (pace / 60) % 60, pace % 60 );
      break;
    case 6 :
      snprintf(lengths_text_to_display,sizeof(lengths_text_to_display),"%d", get_interval_stroke_rate(get_current_interval()));
      break;
 
  }   
 
// Display that text:  
   
text_layer_set_text(lengths_layer,lengths_text_to_display);
}

void update_status_display(void) {
  
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

static void timer_callback(void *data) { // main loop to collect acceleration data
 

static int total_acc;
static time_t t_s;
static uint16_t t_ms;
static int current_time;  
AccelData accel = (AccelData) { .x = 0, .y = 0, .z = 0, .did_vibrate = false, .timestamp = 0 };
  
  if (paused) return; // end here if not swimming 

accel_service_peek(&accel); //get accelerometer data

time_ms(&t_s, &t_ms); 
current_time = t_s*1000 + t_ms; //get current time in ms
  
if (!accel.did_vibrate) { //ignore readings polluted by vibes (1st few always are as start sets off vibration)
  
  total_acc = get_sqrt(square(accel.x)+square(accel.y)+square(accel.z)); //calculate overall acc using pythagoras
  count_strokes(total_acc, current_time);  // call the stroke count function
 // detect_glide(accel.x,current_time); // call the glide detection function

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
    update_elapsed_time_display(NULL);
    timer_callback(0);
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) { //toggle main display
  
  
  
  main_display_setting++;
  if (main_display_setting > 6) main_display_setting = 0;
  
  
 update_status_display(); 
 update_main_display();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) { //toggle pool length
  static char pool_text_to_display [5]; 
  
  
  current_pool_length++;
  if (current_pool_length > number_of_pool_lengths-1) current_pool_length = 0;
  
  snprintf(pool_text_to_display,sizeof(pool_text_to_display),"%dm", pool_length[current_pool_length]);
  text_layer_set_text(pool_layer,pool_text_to_display);
}

static void long_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  
  
  // RESET!!!!! 
  if (paused) {
    set_current_length(1);
    set_length(1,0,0,0); // clear data in current length - leave program to ovewrite other data
    set_current_interval(1);
    set_interval(1,1,1);
    vibes_long_pulse();
    update_main_display();
    update_elapsed_time_display();
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
 // if (get_current_length() == 1) set_length(1,time(NULL),0,0); // this just to avoid dirty data on screen when clock initialised
  show_main_window();
  update_main_display();
 // update_elapsed_time_display();
  
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