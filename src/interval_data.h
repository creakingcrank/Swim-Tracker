#pragma once

int new_interval(void);
int set_interval(int int_number, int start_length, int end_length);
int get_current_interval(void);
int get_interval_first_length(int index);
int get_interval_last_length(int index);
int get_interval_lengths(int index);
int get_interval_duration(int index);
int get_interval_strokes(int index);
int get_interval_stroke_rate(int index);
int get_interval_pace(int index);