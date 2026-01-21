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
    sound_waveform_e waveform;
    uint16_t frequency;
    float duration_seconds;
    int32_t duration_samples;
    float volume;
    ADSRf_t ADSR;
    int16_t sweep;
    vibrato_t vibrato;
    int8_t square_duty_cycle;
    int16_t square_duty_cycle_sweep;
    const sound_t *next;
};
#define SoundFXPlay_args_default .frequency = 500, .volume = 1.0, .ADSR = {.peak = 1, .sustain = .75, .attack = .005, .decay = .002, .release = .005}
void SoundFXPlay_ (SoundFXPlay_args args);
#define SoundFXPlay(...) SoundFXPlay_((SoundFXPlay_args){SoundFXPlay_args_default, __VA_ARGS__})

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

typedef struct {
    uint16_t sent;
    uint8_t signals[256]; // Ring buffer
} sound_signals_t;

void SoundFXPlayPrepared ();