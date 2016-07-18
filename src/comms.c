#include <pebble.h>
#include "comms.h"
#include "length_data.h"
#include "interval_data.h"

void dump_data_to_app_log(void) {
 
  int i;
  int j;
  int current_interval = get_current_interval();
  int zero_time = get_length_start_time(1);
  APP_LOG(APP_LOG_LEVEL_INFO,", Length, Interval, Start, End, Strokes");
  for (i = 1; i<=current_interval; i++) {
    for (j = get_interval_first_length(i); j <= get_interval_last_length(i); j++) {
      APP_LOG(APP_LOG_LEVEL_INFO,",%d,%d,%d,%d,%d", j,i,(int)get_length_start_time(j)-zero_time,(int)get_length_end_time(j)-zero_time,get_length_strokes(j));
    }
  }
  
  
}