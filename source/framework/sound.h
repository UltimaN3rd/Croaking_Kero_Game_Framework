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

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "log.h"

#include "framework_types.h"
#include "c23defs.h"

typedef struct {
    float peak;
    float attack;
    float decay;
    float sustain;
    float release;
} ADSRf_t;

ADSR_t ADSRf_to_ADSR (ADSRf_t in);

void *Sound(void*);

typedef struct SoundFXPlay_args SoundFXPlay_args;
struct SoundFXPlay_args {
    const sound_sample_t *sample;
    uint16_t frequency, frequency_end;
    float duration;
    float volume;
    ADSRf_t ADSR;
    SoundFXPlay_args *next;
};
#define SoundFXPlay_args_default .frequency = 500, .frequency_end = UINT16_MAX, .duration = 0.25, .volume = 1.0, .ADSR = {.peak = .5, .sustain = .4, .attack = .01, .decay = .005, .release = .01}
void SoundFXPlay_ (SoundFXPlay_args args);
#define SoundFXPlay(...) SoundFXPlay_((SoundFXPlay_args){SoundFXPlay_args_default, __VA_ARGS__})

#define ADSR_DEFAULT .peak = .5, .sustain = .4, .attack = 48000 * .01, .decay = 48000 * .005, .release = 48000 * .01

sound_t SoundGenerate_ (SoundFXPlay_args args);
#define SoundGenerate(...) SoundGenerate_((SoundFXPlay_args){SoundFXPlay_args_default, __VA_ARGS__})

void SoundFXPlayDirect (const sound_t sound);

// Returns a bit field of which channels were used to play the sound group
uint32_t SoundFXPlayGroup (const sound_group_t *sounds);

void SoundStopChannel (int channel);

void SoundFXPrepare (const sound_t *sound);

void SoundMusicPlay (const sound_music_t *music);

void SoundMusicStop ();
void SoundMusicResume ();
void SoundMusicSetVolume (float volume_0_to_1);
float SoundMusicGetVolume ();
void SoundFXStop ();
void SoundFXSetVolume (float volume_0_to_1);
float SoundFXGetVolume ();

void SoundStopAll ();

typedef struct {
    bool quit;
    bool prepared_sounds_ready;
} sound_extern_t;

extern sound_extern_t sound_extern_data;

void SoundFXPlayPrepared ();

typedef struct {
    #define SOUND_CHANNELS 26
    sound_t channels[SOUND_CHANNELS];
    struct {
        float volume;
    } fx;
    struct {
        const sound_music_t *new_source;
        float volume;
        enum {music_state_pause, music_state_playing} state;
        sound_music_t source;
    } music;
} sound_internal_t;