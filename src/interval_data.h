#pragma once

int new_interval(void);
int set_current_interval(int interval);
int set_interval(int int_number, int start_length, int end_length);
int get_current_interval(void);
int get_interval_first_length(int index);
int get_interval_last_length(int index);
int get_interval_lengths(int index);
int get_interval_start_time(int index);
int get_interval_duration(int index);
int get_interval_strokes(int index);
int get_interval_stroke_rate(int index);
int get_interval_pace(int index);
void dump_intervals_to_persist(int interval_storage_key_start);
void read_intervals_from_persist(int interval_storage_key_start);
