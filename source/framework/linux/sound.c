// Copyright [2025] [Nicholas Walton]
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "sound.h"
#include "sound_common.h"
#include "framework.h"

#include <pulse/simple.h>
#include <pulse/error.h>

// 5ms period size
#define PERIOD_SIZE (SAMPLING_RATE/200)

void *Sound (void *data_void) {
    sample_buffer_size = PERIOD_SIZE;
    RefillSampleBuffer ();

    pa_simple *simple = NULL;
    int pulse_error;
    { simple = pa_simple_new (NULL, GAME_TITLE, PA_STREAM_PLAYBACK, NULL, "playback", &(pa_sample_spec){.format = PA_SAMPLE_FLOAT32LE, .rate = SAMPLING_RATE, .channels = 1}, NULL, &(pa_buffer_attr){.maxlength = PERIOD_SIZE * 16, .tlength = PERIOD_SIZE * 8, .prebuf = PERIOD_SIZE * 4, .minreq = -1, .fragsize = PERIOD_SIZE * 4}, &pulse_error); if (simple == NULL) { LOG ("Failed to initialize PulseAudio [%s]", pa_strerror (pulse_error)); return NULL; }}

    while (!sound_extern_data.quit) {
        RefillSampleBuffer ();
        int buffer_index = 0;
        { auto result = pa_simple_write (simple, sample_buffer[sample_buffer_swap].samples, PERIOD_SIZE * sizeof (f32), &pulse_error); if (result != 0) { LOG ("PulseAudio failed to write buffer [%s]", pa_strerror (pulse_error)); return NULL; } }
    }

    LOG ("Render thread exiting normally");

    return NULL;
}
