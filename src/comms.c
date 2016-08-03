#include <pebble.h>
#include "comms.h"
#include "length_data.h"
#include "interval_data.h"
#include "pool_data.h"



// Definitions for Persistent storage:
const uint32_t storage_version_key = 1;
const int current_storage_version = 1;
const int current_length_storage_key = 2;
const int current_interval_storage_key = 3;
const int pool_storage_key = 4;
const int length_storage_key_start = 100;
const int interval_storage_key_start = 1000;
 

// Global variables for comms
static int length_to_send = 1;




// Write message to buffer & send
void send_length_data_to_phone(void){
  
  
	DictionaryIterator *iter;
  
	if (length_to_send < get_current_length()) {
    
	  app_message_outbox_begin(&iter);
    
	  dict_write_int16(iter, MESSAGE_KEY_Length, length_to_send);
    dict_write_int16(iter, MESSAGE_KEY_Interval, get_interval_from_length(length_to_send)); 
    dict_write_uint32(iter, MESSAGE_KEY_Start, get_length_start_time(length_to_send));
    dict_write_uint32(iter, MESSAGE_KEY_End, get_length_end_time(length_to_send));
    dict_write_int16(iter, MESSAGE_KEY_Strokes, get_length_strokes(length_to_send));
                    
    if (length_to_send == get_current_length()-1) {
      dict_write_int16(iter, MESSAGE_KEY_Status, 200);
    }
    dict_write_end(iter);
    app_message_outbox_send();
    }
  
}

// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *received, void *context) {
	
  
  Tuple *tuple;
  
  
	tuple = dict_find(received, MESSAGE_KEY_Status);
	if(tuple) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Status: %d -  I hear you JS!", (int)tuple->value->uint32); 
	}
	
	tuple = dict_find(received, MESSAGE_KEY_Message);
	if(tuple) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Message: %s", tuple->value->cstring);
	}
  
// send next length
  send_length_data_to_phone();
}



// Called when an incoming message from PebbleKitJS is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {	
}

// Called when PebbleKitJS does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
}

static void out_success_handler(DictionaryIterator *success, void *context) {

  length_to_send++;
  send_length_data_to_phone();
}

 void init_comms(void) {

	// Register AppMessage handlers
	app_message_register_inbox_received(in_received_handler); 
	app_message_register_inbox_dropped(in_dropped_handler); 
	app_message_register_outbox_failed(out_failed_handler);
  app_message_register_outbox_sent(out_success_handler);

  // Initialize AppMessage inbox and outbox buffers with a suitable size
  const int inbox_size = 128;
  const int outbox_size = 128;
	app_message_open(inbox_size, outbox_size);
}

void reset_comms(void) {
  length_to_send = 1;
}



void dump_data_to_app_log(void) {
 
  int i;
  int j;
  int current_interval = get_current_interval();
  int zero_time = get_length_start_time(1);
  APP_LOG(APP_LOG_LEVEL_INFO,"Garbage, Length, Interval, Start, End, Strokes");
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