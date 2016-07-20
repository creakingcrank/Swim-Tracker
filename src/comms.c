#include <pebble.h>
#include "comms.h"
#include "length_data.h"
#include "interval_data.h"
#include "pool_data.h"

const uint32_t storage_version_key = 1;
  const int current_storage_version = 1;
  const int current_length_storage_key = 2;
  const int current_interval_storage_key = 3;
  const int pool_storage_key = 4;
  const int length_storage_key_start = 100;
  const int interval_storage_key_start = 1000;
  


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

void dump_data_to_persist(void) {

  

  persist_write_int(storage_version_key,current_storage_version);
  persist_write_int(current_length_storage_key,get_current_length());
  persist_write_int(current_interval_storage_key,get_current_interval());

  dump_lengths_to_persist(length_storage_key_start);
  dump_intervals_to_persist(interval_storage_key_start);
  write_pool_to_persist(pool_storage_key);

}

void read_data_from_persist(void) {

  if (persist_read_int(storage_version_key)==current_storage_version) {
    set_current_length(persist_read_int(current_length_storage_key));
    set_current_interval(persist_read_int(current_interval_storage_key));
  
    read_lengths_from_persist(length_storage_key_start);
    read_intervals_from_persist(interval_storage_key_start);
    read_pool_from_persist(pool_storage_key);
  }
  
}