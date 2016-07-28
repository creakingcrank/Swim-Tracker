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
bottom centre of display shows 'w' for workout stats, 'ix' for related to last interval, number x
###Data dump
Data dumped to APP_LOG in csv format<br>
Garbage, Length no., Interval no., Start time in sec after workout start, End time in sec after workout start<br>
Note turn times not included in length time, so to get real swimming pace in spreadsheets etc. use end of previous length to end of current length if lengths in the same interval
