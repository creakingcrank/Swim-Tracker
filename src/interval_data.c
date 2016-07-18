#include <pebble.h>
#include "interval_data.h"
#include "length_data.h"

#define DEBUG

#define MAX_NUMBER_OF_INTERVALS 255

struct interval_struct {
 int first_length;
 int last_length;
};

static struct interval_struct interval[MAX_NUMBER_OF_INTERVALS+1];
static int current_interval = 1; // The next interval to be recorded

static bool is_valid_interval(int index){
  if ((index > current_interval)||(index < 1)) return false;
  else return true;
}  

int new_interval(void) {
  current_interval++;
  if (current_interval > MAX_NUMBER_OF_INTERVALS) {
    current_interval = 1; // if we have reached the end of storage, roll around
    #ifdef DEBUG 
      APP_LOG(APP_LOG_LEVEL_INFO, "Max intervals exceeded, restarting counter");
    #endif 
  }
  #ifdef DEBUG 
      APP_LOG(APP_LOG_LEVEL_INFO, "New interval %d",current_interval );
    #endif 
  return current_interval;
}

int set_interval(int int_number, int start_length, int end_length) {
  
  if ((int_number > MAX_NUMBER_OF_INTERVALS)||(int_number < 0)) return -1;
  
 
  
  interval[int_number].first_length = start_length;
  interval[int_number].last_length =  end_length;
  
  
  #ifdef DEBUG 
    APP_LOG(APP_LOG_LEVEL_INFO, "Interval %d, %d lengths: %d to %d", int_number, end_length-start_length+1, start_length, end_length);
  #endif  
  
  return 0;
}



int get_current_interval(void) {
  return current_interval;
}

int get_interval_first_length(int index) {
  if (!is_valid_interval(index)) return -1;
  return interval[index].first_length;    
}
      
      
int get_interval_last_length(int index) {
   if (!is_valid_interval(index)) return -1;
  return interval[index].last_length;    
}    

      
int get_interval_lengths(int index) {
   if (!is_valid_interval(index)) return -1;
  return interval[index].last_length-interval[index].first_length+1;
      
}

int get_interval_start_time(int index) {
   if (!is_valid_interval(index)) return -1;
  
  return get_length_start_time(get_interval_first_length(index));
}


int get_interval_duration(int index) {
   if (!is_valid_interval(index)) return -1;
  return get_length_end_time(get_interval_last_length(index)) - get_length_start_time(get_interval_first_length(index));    
}      
      
int get_interval_strokes(int index) {
  
  int i;
  int strokes = 0;
  
   if (!is_valid_interval(index)) return -1;
      
  for (i = get_interval_first_length(index); i<=get_interval_last_length(index); i++) {
    strokes = strokes + get_length_strokes(i);
  }
      
  return strokes;    
}   
      
int get_interval_stroke_rate(int index) {
  
  int strokes;
  int duration;
  
  if (!is_valid_interval(index)) return -1;
      
  strokes = get_interval_strokes(index);
 duration = get_interval_duration(index);
  
      
  return 60 * strokes /duration;    
}     

int get_interval_pace(int index) {
  
  int pace;
  
  if (!is_valid_interval(index)) return -1;
  
  pace = get_interval_duration(index)/get_interval_lengths(index);
  
  return pace;
  
}
