#pragma once

extern sound_extern_t sound_extern_data;

#define SAMPLE_BUFFER_SIZE 1024
typedef struct {
    int16_t played;
    float samples[SAMPLE_BUFFER_SIZE];
    float peak;
} sample_buffer_t;
extern sample_buffer_t sample_buffer[2];
extern bool sample_buffer_swap;
extern int16_t sample_buffer_size;
void RefillSampleBuffer ();
#define SAMPLING_RATE 48000