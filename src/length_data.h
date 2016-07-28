#pragma once
int set_length( int length_number, time_t start, time_t end, int stroke);
int set_current_length(int length);
int get_total_number_of_lengths(void);
int get_current_length(void);
time_t get_length_start_time(int index);
time_t get_length_end_time(int index);
int get_length_strokes(int index);
int get_length_duration(int index);
int get_length_stroke_rate(int index);
int get_workout_duration(void);
int elapsed_time_in_workout(void);
void dump_lengths_to_persist(int length_storage_key_start);
void read_lengths_from_persist(int length_storage_key_start);
