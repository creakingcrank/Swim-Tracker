#include <pebble.h>
#include "length_data.h"
#include "interval_data.h"

#define DEBUG

#define MAX_NUMBER_OF_LENGTHS 255
#define INTERVAL_TRIGGER_TIME_S 10

/* This is the file for handling lengths and intervals storage */

 struct length_struct {
  time_t start_time;
  time_t end_time;
  int strokes;
};

 

static struct length_struct length[MAX_NUMBER_OF_LENGTHS+1];
  
static int current_length = 1; // The next length to be recorded


int set_length( int length_number, time_t start, time_t end, int stroke)  {
  
  int index;
  int current_interval =  get_current_interval();
  
  if (length_number > MAX_NUMBER_OF_LENGTHS) return -1;
  
  if (length_number == 0 ) {
    index = current_length;
    current_length++;
  }
  else index = length_number;
  
  
  length[index].start_time = start;
  length[index].end_time = end;
  length[index].strokes = stroke;
  
  if (current_length > MAX_NUMBER_OF_LENGTHS) current_length = 1; // if we have reached the end of storage, roll around
  
#ifdef DEBUG 
  APP_LOG(APP_LOG_LEVEL_INFO, "Length %d, %d strokes", current_length-1, length[index].strokes);
#endif 
  
  

  /* now check for a new interval */
  
  if (current_length == 2) { //if that was the first length, log the beginning of the first interval
    set_interval(1, 1, 1);
  }
  else if ( (length[index].start_time-length[index-1].end_time) > INTERVAL_TRIGGER_TIME_S ) {
    set_interval(new_interval(), index, index); //start a new interval
  }
  else set_interval(current_interval, get_interval_first_length(current_interval), index); // add this length to the current interval 
  
return current_length-1; // return the total number of lengths recorded so far
}

int get_total_number_of_lengths(void) {
  return current_length - 1;
}

time_t get_length_start_time(int index) {
  if (index > current_length-1) return -1;
  if (index < 1) return -1;
  return length[index].start_time;
}

time_t get_length_end_time(int index) {
  if (index > current_length-1) return -1;
  if (index < 1) return -1;
  return length[index].end_time;
}

int get_length_strokes(int index) {
  if (index > current_length-1) return -1;
  if (index < 1) return -1;
  return length[index].strokes;
}

int get_length_duration(int index) {
  if (index > current_length-1) return -1;
  if (index < 1) return -1;
  return length[index].end_time-length[index].start_time;
}

int get_length_stroke_rate(int index) {
  
  int duration;
  
  if (index > current_length-1) return -1;
  if (index < 1) return -1;
  
  duration = get_length_duration(index);
  
  return 60*length[index].strokes/duration;
  
}