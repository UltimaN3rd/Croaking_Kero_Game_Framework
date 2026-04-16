#pragma once

extern sound_extern_t sound_extern_data;

#define SAMPLE_BUFFER_SIZE_MAX 4096
typedef struct {
    i16 played;
    f32 samples[SAMPLE_BUFFER_SIZE_MAX];
    f32 peak;
} sample_buffer_t;
extern sample_buffer_t sample_buffer[2];
extern bool sample_buffer_swap;
extern i16 sample_buffer_size;
void RefillSampleBuffer ();
#define SAMPLING_RATE 48000