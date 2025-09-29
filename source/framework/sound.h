#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "log.h"

typedef struct {
    float peak;
    uint16_t attack;
    uint16_t decay;
    float sustain;
    uint16_t release;
} ADSR_t;

typedef struct {
    float peak;
    float attack;
    float decay;
    float sustain;
    float release;
} ADSRf_t;

ADSR_t ADSRf_to_ADSR (ADSRf_t in);

#include "samples.h"

typedef struct sound_t {
    uint32_t t;
    uint32_t duration;
    const struct sound_t *next;
    float d;
    int16_t frequency_begin, frequency_end;
    uint8_t ADSR_state;
    ADSR_t ADSR;
    const sound_sample_t *sample;
} sound_t;

#include "c23defs.h"

typedef struct {
    uint8_t count;
    counted_by(count) const sound_t sounds[];
} sound_group_t;

typedef struct {
    uint8_t count;
    counted_by(count) const sound_t *sounds[];
} sound_music_t;

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