#include <pebble.h>
#include "main_window.h"

 #define DEBUG  /* debugging code */

#define ACCEL_STEP_MS 20         /* frequency of accelerometer checks */
#define ACC_THRESHOLD 1100       /* threshold for an acceleration peak - stationary wrist is about 1000 */
#define STROKE_MIN_PEAK_TIME_MS 100     /* minimim duration of peak to count it */
#define MIN_STROKES_PER_LENGTH 5 /* minimum number of recorded strokes for a length to be valid */

/* define constants for glide detection */

#define GLIDE_X_THRESHOLD -200 /* X direction threshold for glide detection */
#define MIN_GLIDE_TIME_MS 1700 /* Trigger a length if no stroke detected this long after glide registered */
#define GLIDE_MIN_PEAK_TIME_MS 400 /* minimum duration of glide to count it */

static int ave_peak_to_peak_time_ms = 2500; // average time between peaks


static int strokes = 0;     //counter for strokes recognised in current length
static int peaks = 0;       //counter for acceleration peaks recognised, 2 peaks makes a stroke
static int lengths =0;    //counter for lengths (a pause pr a glide after several strokes means a length)
static int lengths_in_interval = 0; // count lengths since last clock trigger
static int up_time = 0;     // time current peak started, to discount short peaks from measurement
static int last_peak_time = 0; // measure overall stroke time from beginning of 1st peak
static bool up = false;     // have we logged an acceleration peak?
static bool swimming = false; // toggle to stop fiddling with things until a stroke is counted
static bool paused = true;  // toggled by set button


time_t timer_started = 0; // if the clock is running, this holds the the time it was started -
int elapsed_time = 0; //the number of seconds the clock has been running

time_t length_timer_started = 0; // the time we start registering swimming in a length, for stroke rate etc.
int length_elapsed_time = 0; // the elapsed length time;
int swimming_elapsed_time = 0; // cumulative length time for interval - reset when timer reset, used for pace calculation

static int pool_length[] = {25, 33, 50};
static int number_of_pool_lengths = sizeof(pool_length)/sizeof(pool_length[0]);
static int current_pool_length = 0; //pool lengths toggle by down click

static int main_display_setting = 0; // what we show on the main screen 
                                    // 0 = number of lengths, 1 = distance covered, 2 = average pace /100m, 
                                    // 3 =last length strole rate /min  
                                    // toggle by up click

/*

THINGS TO DO

Automatic intervals?? define a maximum turn time of, say 10 seconds, 
if we don't see a stroke in that time, switch resting on. 

Interval count.

Interval recording (distance, time, strokes)


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
  strokes_layer = text_layer_create(GRect(70, 130, 65, 30));
  text_layer_set_text(strokes_layer, "0.0 str");
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
  text_layer_set_text(l_text, "Len. w");
  text_layer_set_text_alignment(l_text, GTextAlignmentCenter);
  text_layer_set_font(l_text, s_res_bitham_30_black);
  layer_add_child(window_get_root_layer(s_window), (Layer *)l_text);
  
  // pool_layer
  pool_layer = text_layer_create(GRect(5, 130, 60, 30));
  text_layer_set_text(pool_layer, "25m");
  text_layer_set_font(pool_layer, s_res_gothic_28_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)pool_layer);
}

static void destroy_ui(void) {
  window_destroy(s_window);
  text_layer_destroy(lengths_layer);
  text_layer_destroy(strokes_layer);
  text_layer_destroy(stopwatch_layer);
  text_layer_destroy(l_text);
  text_layer_destroy(pool_layer);
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

static void increment_lengths(void) {
  
  if (strokes >= MIN_STROKES_PER_LENGTH) { //If we haven't done MIN_STROKES_PER_LENGTH, we assume length aborted or noisy data 
      lengths++;
      lengths_in_interval++;
      vibes_long_pulse(); // for testing only
      length_elapsed_time = time(NULL) - length_timer_started; // the number of seconds in that last length
      swimming_elapsed_time = swimming_elapsed_time + length_elapsed_time; // number of seconds we have been swimming
      #ifdef DEBUG 
        APP_LOG(APP_LOG_LEVEL_INFO, "Length %d recorded at %ds, %d strokes", lengths, elapsed_time, strokes );
      #endif
    }
    #ifdef DEBUG
    else APP_LOG(APP_LOG_LEVEL_INFO, "Insufficient strokes recorded, restarting length");
    #endif
    
    //reset everything for next length:
    strokes = 0;
    peaks = 0;
    up = false;
    swimming = false;
    update_main_display();
}

static void count_strokes(int accel, int timestamp) {
  
  /*
  
  Here's the algorithm: 
  
  A stroke has two acceleration peaks in quick succession (up and down or out and back) (strokes = peaks /2)
  We count a peak if accel stays above ACC_THRESHOLD for longer than MIN_PEAK_TIME
  For each length, we track the average time between peaks.
  If we don't see another peak for 2 * that average we call the increment lengths function
  
  
  */
  
  if ((accel > ACC_THRESHOLD) && (up == false)) { //record the start of an acc peak
      up = true;
      up_time = timestamp;
    }
  
  if ( (accel < ACC_THRESHOLD) && (up == true) )  { //identify the end of an acc peak
    up = false;
    if (timestamp-up_time > STROKE_MIN_PEAK_TIME_MS) {
      if (peaks > 2)  ave_peak_to_peak_time_ms = ((ave_peak_to_peak_time_ms * peaks)+(timestamp-last_peak_time))/(peaks+1); //ignore first couple of peaks in case noisy
      
      last_peak_time = timestamp;
      peaks++; //if the peak was long enough, log it
      if (peaks == 1) length_timer_started=time(NULL);
      #ifdef DEBUG
        APP_LOG(APP_LOG_LEVEL_INFO, "Peak %d %dms recorded at %ds, %d strokes, ap2p %dms", peaks, timestamp-up_time, elapsed_time, strokes, ave_peak_to_peak_time_ms);
      #endif
      update_main_display(); // this basically for debug so we can see peak tracking on screen
    }
  }    
 
strokes = peaks / 2;
   
update_main_display();

  
  if ( ((timestamp - last_peak_time) > (2 * ave_peak_to_peak_time_ms)) && (strokes > 1)) {
    #ifdef DEBUG
      APP_LOG(APP_LOG_LEVEL_INFO, "Peak missed, triggering length check at %ds", elapsed_time);
    #endif
    increment_lengths();
  }

}

static void update_elapsed_time_display(){
  
  static char time_to_display[9];
  time_t current_time;
  if (timer_started > 0){
    current_time = time(NULL);
    elapsed_time = elapsed_time + current_time - timer_started;
    timer_started = current_time;
    text_layer_set_background_color(stopwatch_layer, GColorWhite);
    text_layer_set_text_color(stopwatch_layer, GColorBlack);
    app_timer_register(1000, update_elapsed_time_display, NULL );
  }
  else {
   text_layer_set_background_color(stopwatch_layer, GColorBlack);
   text_layer_set_text_color(stopwatch_layer, GColorWhite);
  }
  snprintf(time_to_display,sizeof(time_to_display),"%01d:%02d:%02d",elapsed_time / 3600, (elapsed_time / 60) % 60, elapsed_time  % 60 );
  text_layer_set_text(stopwatch_layer, time_to_display);
  
}

 void update_main_display(){
  
  static char lengths_text_to_display [8];
  static char strokes_text_to_display [12];
  int pace = (swimming_elapsed_time/lengths_in_interval)*(100/pool_length[current_pool_length]); // pace of last interval in s/100m, ignoring rest time
 
 // define the text to display in the main text layer:  
   if  ( main_display_setting == 0) {
    snprintf(lengths_text_to_display,sizeof(lengths_text_to_display),"%d", lengths);
  }
    if ( main_display_setting == 1) {
    snprintf(lengths_text_to_display,sizeof(lengths_text_to_display),"%dm", lengths*pool_length[current_pool_length]);
  }
   if  ( main_display_setting == 2) {
    snprintf(lengths_text_to_display,sizeof(lengths_text_to_display),"%d", lengths_in_interval);
  }
  if ( main_display_setting == 3) {
    snprintf(lengths_text_to_display,sizeof(lengths_text_to_display),"%dm", lengths_in_interval*pool_length[current_pool_length]);
  } 
if ( main_display_setting == 4) {
  snprintf(lengths_text_to_display,sizeof(lengths_text_to_display),"%01d:%02d", (pace / 60) % 60, pace % 60 );
}
if ( main_display_setting == 5) {
  snprintf(lengths_text_to_display,sizeof(lengths_text_to_display),"%d", 30000/ave_peak_to_peak_time_ms); 
}   
 
// Display that text:  
   
text_layer_set_text(lengths_layer,lengths_text_to_display);

// Next section really for debug, provide some info on strokes counted etc. :
   
  snprintf(strokes_text_to_display,sizeof(strokes_text_to_display),"%d str", strokes);
  text_layer_set_text(strokes_layer,strokes_text_to_display);

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
  
  paused = !paused;
  vibes_double_pulse();
  #ifdef DEBUG
    APP_LOG(APP_LOG_LEVEL_INFO,"Timer triggered at %ds",elapsed_time);
  #endif
  
  if (!paused){
    timer_started = time(NULL);
    swimming_elapsed_time = 0; // reset swimming elapsed time so pace shows pace of last interval, more useful?
    lengths_in_interval = 0;
    update_elapsed_time_display();
  }
  else {
    timer_started = 0;
    strokes = 0;
    peaks = 0;
    update_main_display();
  }
  
  
  timer_callback(0);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) { //toggle main display
  
  if (!paused) { // if button pressed while clock running, used to correct length recording errors.
    lengths--;
    if (lengths<0) lengths = 0;
    update_main_display();
    return;
  }
  
  main_display_setting++;
  if (main_display_setting > 5) main_display_setting = 0;
  
  switch (main_display_setting) {
    case 0 :
      text_layer_set_text(l_text, "Len. w");
      break;
    case 1 :
      text_layer_set_text(l_text, "Dist. w");
      break;
    case 2 :
      text_layer_set_text(l_text, "Len. i");
      break;
     case 3 :
      text_layer_set_text(l_text, "Dist. i");
      break;
    case 4 :
      text_layer_set_text(l_text, "Pace i");
      break;
    case 5 :
      text_layer_set_text(l_text, "Str. rate");
      break;
  }
  update_main_display();
 
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) { //toggle pool length
  static char pool_text_to_display [5]; 
  
  if (!paused) { // if button pressed while clock running, used to correct length recording errors.
    lengths++;
    update_main_display();
    return;
  }
  
  current_pool_length++;
  if (current_pool_length > number_of_pool_lengths-1) current_pool_length = 0;
  
  snprintf(pool_text_to_display,sizeof(pool_text_to_display),"%dm", pool_length[current_pool_length]);
  text_layer_set_text(pool_layer,pool_text_to_display);
  update_main_display();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
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
  show_main_window();
}

static void deinit() {
  
//Window destroyed automatically so no action here at the moment, exists to destroy data in future

}


int main(void) {
  
  init();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);
  app_event_loop();
  deinit();
  
}