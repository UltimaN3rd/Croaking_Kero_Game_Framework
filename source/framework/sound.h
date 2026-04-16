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
    f32 peak;
    f32 attack;
    f32 decay;
    f32 sustain;
    f32 release;
} ADSRf_t;

ADSR_t ADSRf_to_ADSR (ADSRf_t in);

void *Sound(void*);

typedef struct SoundFXPlay_args SoundFXPlay_args;
struct SoundFXPlay_args {
    sound_waveform_e waveform;
    u16 frequency;
    f32 duration_seconds;
    i32 duration_samples;
    f32 volume;
    ADSRf_t ADSR;
    i16 sweep;
    vibrato_t vibrato;
    i8 square_duty_cycle;
    i16 square_duty_cycle_sweep;
    const sound_t *next;
};
#define SoundFXPlay_args_default .frequency = 500, .volume = 1.0, .ADSR = {.peak = 1, .sustain = .75, .attack = .005, .decay = .002, .release = .005}
void SoundFXPlay_ (SoundFXPlay_args args);
#define SoundFXPlay(...) SoundFXPlay_((SoundFXPlay_args){SoundFXPlay_args_default, __VA_ARGS__})

sound_t SoundGenerate_ (SoundFXPlay_args args);
#define SoundGenerate(...) SoundGenerate_((SoundFXPlay_args){SoundFXPlay_args_default, __VA_ARGS__})

void SoundFXPlayDirect (const sound_t sound);

// Returns a bit field of which channels were used to play the sound group
u32 SoundFXPlayGroup (const sound_group_t *sounds);

void SoundStopChannel (int channel);

void SoundFXPrepare (const sound_t *sound);

void SoundMusicPlay (const sound_music_t *music);

void SoundMusicStop ();
void SoundMusicResume ();
void SoundMusicSetVolume (f32 volume_0_to_1);
f32 SoundMusicGetVolume ();
void SoundFXStop ();
void SoundFXSetVolume (f32 volume_0_to_1);
f32 SoundFXGetVolume ();

void SoundStopAll ();

typedef struct {
    bool quit;
    bool prepared_sounds_ready;
} sound_extern_t;

extern sound_extern_t sound_extern_data;

typedef struct {
    u16 sent;
    u8 signals[256]; // Ring buffer
} sound_signals_t;

void SoundFXPlayPrepared ();