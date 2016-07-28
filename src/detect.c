#include <pebble.h>
#include "detect.h"
#include "length_data.h"
#include "interval_data.h"

#define DEBUG /* Debugging code */

#define ACC_THRESHOLD 1100       /* threshold for an acceleration peak - stationary wrist is about 1000 */
#define STROKE_MIN_PEAK_TIME_MS 150     /* minimim duration of peak to count it */
#define MIN_STROKES_PER_LENGTH 5 /* minimum number of recorded strokes for a length to be valid, current used in a display function only */ 
#define TRIGGER_AUTO_INTERVAL_AFTER_S 10 /* number of seconds to wait before auto interval trigger */
#define AVE_STROKES_PER_LENGTH_FLOOR 12 /* A seed number for average strokes per length, used for 1st length in interval only, after that, we use real ave */
#define INITIAL_AVERAGE_PEAK_TO_PEAK_TIME_MS 1000 /* a seed number for the avergae time between peaks, used for length end sensing, adapted during swimming */

// define constants for missing stroke detection
#define MAX_AVE_PEAK_TO_PEAK_TIME_MS 2500 // the initial, and maximum allowed, average time between peaks
#define MISSING_PEAK_SENS 9/4 // The number of average peak gaps we wait before a length check - integer calculation so use fractions here


static bool length_end_check(int strokes, int ave_strokes_per_length) {
  
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
    
    
return return_value;
}


static int get_missing_peak_window(int ap2p, int astrokes, int stroke) {
  
  /* 
  function to return a time window for length end detection.
  Window gets smaller as length progresses
  No peak detected in the window = a length turn
  */
  
  static int base_sense_percent = 200; // this is the minimum ap2p multiplier, should never be less than 200
  static int variable_sense_percent = 200; // this is the variable element, falls through the length
  int missing_peak_window;
  
  int percent_of_length_left = 0;
  
  
  if (stroke < astrokes) percent_of_length_left = 100*(astrokes - stroke)/astrokes;
  
  missing_peak_window =  ap2p * (base_sense_percent + (variable_sense_percent*percent_of_length_left/100)) / 100;
  
  return missing_peak_window;
  
}

bool count_strokes(int accel, int timestamp) {
  
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
 static int ave_strokes_per_length = AVE_STROKES_PER_LENGTH_FLOOR; // Average number of strokes per length, learned during swim 
 static int up_time = 0;     // time current peak started, to discount short peaks from measurement
 static int last_peak_time = 0; // measure overall stroke time from beginning of 1st peak
 static bool up = false;     // have we logged an acceleration peak?
 static time_t start_of_length_time; // The time the first acceleration peak in a length is recorded 
  
 bool  return_value = false; //returns true if end of length identified and display needs updating
   

  
  if ((accel > ACC_THRESHOLD) && (up == false)) { //identify the start of an acc peak
      up = true;
      up_time = timestamp;
    }
  
  if ( (accel < ACC_THRESHOLD) && (up == true) )  { //identify the end of an acc peak
    up = false;
    
    if (timestamp-up_time > STROKE_MIN_PEAK_TIME_MS) { // was it long enough to count as a peak?
      
      latest_peak_to_peak_time_ms = timestamp - last_peak_time;
      
      if (peaks>6) { // Ignore first few peaks so that movement between lengths doesn't affect ap2p
        
        ave_peak_to_peak_time_ms = ((ave_peak_to_peak_time_ms * (peaks-1)) + latest_peak_to_peak_time_ms  )/ peaks; 
        
        if (ave_peak_to_peak_time_ms > MAX_AVE_PEAK_TO_PEAK_TIME_MS) ave_peak_to_peak_time_ms = MAX_AVE_PEAK_TO_PEAK_TIME_MS; // if avep2p gets too big, cap it.
        
      }
      
      last_peak_time = timestamp;
      peaks++; //if the peak was long enough, log it
      strokes = peaks / 2;
      if (peaks == 1) start_of_length_time=time(NULL);
      #ifdef DEBUG
        APP_LOG(APP_LOG_LEVEL_INFO, "Peak %d %dms recorded at %ds, %d strokes, p2p %dms, ap2p %dms", peaks, 
                timestamp-up_time, elapsed_time_in_workout(), strokes, latest_peak_to_peak_time_ms, ave_peak_to_peak_time_ms);
      #endif
    }
  }    
 

   
  // check for end of length
  
  if ( ((timestamp - last_peak_time) > get_missing_peak_window(ave_peak_to_peak_time_ms, ave_strokes_per_length, strokes)) && (peaks > 0) ) {
    #ifdef DEBUG
      APP_LOG(APP_LOG_LEVEL_INFO, "Peak missed, triggering length check at %ds", elapsed_time_in_workout());
    #endif
    if (length_end_check(strokes, ave_strokes_per_length)) {
      set_length( 0, start_of_length_time, time(NULL), strokes);
      return_value = true;
      if (ave_strokes_per_length < AVE_STROKES_PER_LENGTH_FLOOR) ave_strokes_per_length = AVE_STROKES_PER_LENGTH_FLOOR; // Keep ave strokes per length sensible
    }
    up = false;
    strokes = 0;
    peaks = 0;
    
  }
return return_value;
}