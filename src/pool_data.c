#include <pebble.h>
#include "pool_data.h"

static int pool[] = {25, 33, 50};
static int number_of_pools = sizeof(pool)/sizeof(pool[0]);
static int current_pool = 0; 

int pool_length(int change) {
  
  current_pool = current_pool + change;
  if (current_pool>(number_of_pools-1)) current_pool = 0;
  if (current_pool<0) current_pool = number_of_pools-1;
  
  return pool[current_pool];
}

void write_pool_to_persist(int key) {
  persist_write_int(key,current_pool);
}

void read_pool_from_persist(int key) {
  current_pool = persist_read_int(key);
}