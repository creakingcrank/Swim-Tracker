# Swim-Tracker
Accelerometer based swim tracker for Pebble watches <br>
## Instructions<br>
Select - starts or stops timer (timer background white when running, black when stopped, elapsed timer records swim time only, so won't change until a length has been swum)<br>
Up  - toggles display options<br>
Down - adjusts pool length (25mm/33m/50m, pool length shown bottom left of display)<br>
Long press Down (3sec) dumps data to APP_LOG<br>
Long press Set (3sec) resets data if timer stopped (buzz on reset)
### Display options (Up button)<br>
[workout]Lengths -> Distance -> Pace -> [Last interval] Lengths -> Distance -> Pace -> Stroke rate<br>
bottom centre of display shows 'w' for workout stats, 'ix' for stats related to last interval, number x
###Intervals
Interval count triggered automatically if new length not started 10 seconds after last length end<br>
To manually trigger an interval change, toggle timer off/on (Double press on Select)
###Data dump
Data dumped to APP_LOG in csv format<br>
Garbage, Length no., Interval no., Start time in sec after workout start, End time in sec after workout start<br>
Note turn times not included in length time, so to get real swimming pace in spreadsheets etc. use end of previous length to end of current length if lengths in the same interval
### Persistent storage
Length and interval data held in persistent storage, so OK to leave app and return later to dump data. Don't forget to reset data before next swim
### Stroke counts
Stoke rate/counts are for the arm with the watch on it, so freestyle/backstroke swimmers may want to double these numbers to get an approximation of their stroke rates and counts if they habitually count per arm, rather for a full stroke cycle.
