#pragma once
int set_length( int length_number, time_t start, time_t end, int stroke);
int get_total_number_of_lengths(void);
time_t get_length_start_time(int index);
time_t get_length_end_time(int index);
int get_length_strokes(int index);
int get_length_duration(int index);
int get_length_stroke_rate(int index);