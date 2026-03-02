#include "sound.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "turns_math.h"

#define MUSIC_CHANNELS_FIRST 0
#define MUSIC_CHANNELS 10
#define MUSIC_CHANNELS_LAST (MUSIC_CHANNELS - 1)
#define FX_CHANNELS_FIRST MUSIC_CHANNELS
#define FX_CHANNELS 10
#define FX_CHANNELS_LAST (MUSIC_CHANNELS + FX_CHANNELS - 1)
#define SOUND_CHANNELS (MUSIC_CHANNELS + FX_CHANNELS)

sound_extern_t sound_extern_data;

typedef struct {
    sound_t sound;
    uint8_t ADSR_state;
    uint32_t t;
    float d; // Wave position where 0 = start, 1 = full period. Not reset to 0 when sound channels change, in order to transition between waves smoothly. This value is like degrees or radians, but measures turns.
	float vibrato_d; // Same as d, but for vibrato
    union {
        struct {
            int16_t current_duty_cycle;
        } pulse;
    } u;
} sound_channel_t;

typedef enum {music_state_pause, music_state_playing} music_state_e;

typedef struct {
    sound_channel_t channels[SOUND_CHANNELS];
    struct {
        float volume;
    } fx;
    float master_volume;
    struct {
        const sound_music_t *new_source;
        float volume;
        music_state_e state;
        sound_music_t source;
    } music;
} sound_internal_t;

static sound_internal_t sound = {
    .fx = {
        .volume = 1,
    },
    .master_volume = .5,
    .music = {
        .volume = 1,
    },
};

typedef enum { sound_command_fx_stop, sound_command_fx_play, sound_command_fx_prepare, sound_command_fx_play_prepared, sound_command_fx_volume_set, sound_command_music_new, sound_command_music_pause, sound_command_music_resume, sound_command_music_volume_set } sound_command_e;

static struct {
    uint8_t executed, filled; // when filled < executed, execute until overflow, then check again.
    // SIZE must be 256 so the overflow logic works
    #define COMMAND_BUFFER_SIZE 256
    struct {
        sound_command_e type;
        union {
            struct {
                sound_t sound;
            } fx_play;
            struct {
                const sound_t *sound;
            } fx_prepare;
            struct {
                float volume;
            } fx_volume_set;
            struct {
                const sound_music_t *music;
            } music_new;
            struct {
                float volume;
            } music_volume_set;
        } data;
    } commands[COMMAND_BUFFER_SIZE];
} command_buffer;

#define SoundAddCommand(...) _SoundAddCommand ((typeof(command_buffer.commands[0])){__VA_ARGS__})
static void _SoundAddCommand (typeof(command_buffer.commands[0]) command) {
    int i = command_buffer.filled % COMMAND_BUFFER_SIZE;
    command_buffer.commands[i] = command;
    ++command_buffer.filled;
}

static inline int SelectFXChannel () {
    for (int i = FX_CHANNELS_FIRST; i <= FX_CHANNELS_LAST; ++i) {
        if (sound.channels[i].sound.waveform == sound_waveform_none) {
            return i;
        }
    }
    return -1;
}

static inline void SoundExecuteCommands () {
    uint8_t filled = command_buffer.filled; // Latch this value since other threads could be modifying it while we work through commands
    uint8_t count = filled - command_buffer.executed;
    if (filled < command_buffer.executed) { // Filled has looped around so we need to execute the remaining commands
        count = COMMAND_BUFFER_SIZE - command_buffer.executed + filled;
    }
    while (count--) {
        typeof(command_buffer.commands[0]) c = command_buffer.commands[command_buffer.executed++];
        switch (c.type) {
            case sound_command_fx_stop: {
                for (int i = FX_CHANNELS_FIRST; i <= FX_CHANNELS_LAST; ++i)
                    sound.channels[i].sound.waveform = sound_waveform_none;
            } break;
            case sound_command_fx_play: {
                int selected_channel = SelectFXChannel ();
                if (selected_channel == -1) break;
                sound.channels[selected_channel] = (typeof(sound.channels[selected_channel])){ .sound = c.data.fx_play.sound };
            } break;
            case sound_command_fx_prepare: {
                int selected_channel = SelectFXChannel ();
                if (selected_channel == -1) break;
                memset (&sound.channels[selected_channel], 0, sizeof (sound.channels[selected_channel]));
                sound.channels[selected_channel].sound.waveform = sound_waveform_preparing;
                sound.channels[selected_channel].sound.duration = UINT32_MAX;
                sound.channels[selected_channel].sound.next = c.data.fx_prepare.sound;
            } break;
            case sound_command_fx_play_prepared: {
                sound_extern_data.prepared_sounds_ready = true;
            } break;
            case sound_command_fx_volume_set: {
                assert (c.data.fx_volume_set.volume >= 0);
                assert (c.data.fx_volume_set.volume <= 1);
                sound.fx.volume = c.data.fx_volume_set.volume;
            } break;
            case sound_command_music_new: {
                sound.music.state = music_state_playing;
                sound.music.new_source = c.data.music_new.music;
                for (int c = MUSIC_CHANNELS_FIRST; c <= MUSIC_CHANNELS_LAST; ++c)
                    sound.channels[c].sound.waveform = sound_waveform_silence;
                int channel_count = sound.music.new_source->count;
                if (channel_count > MUSIC_CHANNELS) { LOG ("New music requested has too many channels [%d] > [%d]", channel_count, MUSIC_CHANNELS); channel_count = MUSIC_CHANNELS; }
                for (int c = 0; c < channel_count; ++c)
                    sound.channels[c] = (typeof(sound.channels[c])){.sound = *sound.music.new_source->sounds[c]};
            } break;
            case sound_command_music_pause: {
                sound.music.state = music_state_pause;
            } break;
            case sound_command_music_resume: {
                sound.music.state = music_state_playing;
            } break;
            case sound_command_music_volume_set: {
                sound.music.volume = c.data.music_volume_set.volume;
            } break;
        }
    }
}

void SoundMusicStop () {
    SoundAddCommand (.type = sound_command_music_pause);
}
void SoundMusicResume () {
    SoundAddCommand (.type = sound_command_music_resume);
}

void SoundMusicSetVolume (float volume_0_to_1) {
    if (volume_0_to_1 < 0) volume_0_to_1 = 0;
    if (volume_0_to_1 > 1) volume_0_to_1 = 1;
    SoundAddCommand (.type = sound_command_music_volume_set, .data.music_volume_set.volume = volume_0_to_1);
}
float SoundMusicGetVolume () { return sound.music.volume; }

void SoundFXStop () {
    SoundAddCommand (.type = sound_command_fx_stop);
}
void SoundFXSetVolume (float volume_0_to_1) {
    if (volume_0_to_1 < 0) volume_0_to_1 = 0;
    if (volume_0_to_1 > 1) volume_0_to_1 = 1;
    SoundAddCommand (.type = sound_command_fx_volume_set, .data.fx_volume_set.volume = volume_0_to_1);
}
float SoundFXGetVolume () { return sound.fx.volume; }

#define SAMPLING_RATE 48000
#ifdef __linux__
#define PERIOD_SIZE (SAMPLING_RATE / 200)
#elifdef WIN32
#define PERIOD_SIZE (SAMPLING_RATE / 200)
#elifdef __APPLE__
#define PERIOD_SIZE 240
#endif

// Two 5ms buffers
// SAMPLE_BUFFER_SIZE is the maximum size allowed. On Linux we only use 240 (5ms). Windows must be discovered at runtime, seems to usually be 256. Mac lets us set it to 240 but can reset itself.
// Currently one buffer holds 5ms uncompressed samples, the next buffer holds 5ms compressed. When the audio system requests samples, we copy over the compressed ones, compress the uncompressed and generate 5ms of new samples. If there are problems with variable chunk sizes in future, we could just generate some max amount of samples (10ms or so?) then compress the amount requested and refill exactly that amount. Just have to track the chunk sizes to lerp compression value correctly.
#define SAMPLE_BUFFER_SIZE 1024
struct {
    int16_t played;
    float samples[SAMPLE_BUFFER_SIZE];
    float peak;
} sample_buffer[2] = {};
bool sample_buffer_swap = 0; // Use this to index the buffer which is compressed and ready for playback.
int16_t sample_buffer_size = 0;

ADSR_t ADSRf_to_ADSR (ADSRf_t in) { return (ADSR_t){.peak = in.peak, .attack = in.attack * SAMPLING_RATE, .decay = in.decay * SAMPLING_RATE, .sustain = in.sustain, .release = in.release * SAMPLING_RATE}; }

sound_t SoundGenerate_(SoundFXPlay_args args) {
    assert (args.waveform);
    if (args.waveform == sound_waveform_none) return (sound_t){};
    args.ADSR.peak *= args.volume;
    args.ADSR.sustain *= args.volume;
    uint32_t duration = args.duration_samples;
    if (duration == 0) duration = args.duration_seconds * SAMPLING_RATE;
    sound_t sound = {
        .waveform = args.waveform,
        .duration = duration,
        .next = args.next,
        .ADSR = ADSRf_to_ADSR(args.ADSR),
        .frequency = args.frequency,
        .sweep = args.sweep,
        .vibrato = args.vibrato,
        .square_duty_cycle = args.square_duty_cycle,
        .square_duty_cycle_sweep = args.square_duty_cycle_sweep,
    };
    // if (sound.waveform == sound_waveform_pulse) {
    //     if (sound.vibrato.frequency_range < sound.frequency * .005f) sound.vibrato.frequency_range = sound.frequency * .005f;
    //     if (sound.vibrato.vibrations_per_hundred_seconds < sound.frequency/20) sound.vibrato.vibrations_per_hundred_seconds = sound.frequency/20;
    // }
    int duration_ADSR = sound.ADSR.attack + sound.ADSR.decay + sound.ADSR.release;
    if (sound.duration < duration_ADSR) { // There isn't enough time for the full envelope, even with 0 sustain. Get ratio of envelope and apply to the reduced duration
        float total = args.ADSR.attack + args.ADSR.decay + args.ADSR.release;
        sound.ADSR.attack = (args.ADSR.attack / total) * sound.duration;
        sound.ADSR.release = (args.ADSR.release / total) * sound.duration;
        sound.ADSR.decay = sound.duration - sound.ADSR.attack - sound.ADSR.release;
    }
    return sound;
}

void SoundFXPlay_ (SoundFXPlay_args args) {
    SoundAddCommand (.type = sound_command_fx_play, .data.fx_play.sound = SoundGenerate_ (args));
}

void SoundFXPlayDirect (const sound_t sound) {
    SoundAddCommand (.type = sound_command_fx_play, .data.fx_play.sound = sound);
}

void SoundFXPrepare (const sound_t *const sound) {
    SoundAddCommand (.type = sound_command_fx_prepare, .data.fx_prepare.sound = sound);
}

uint32_t SoundFXPlayGroup (const sound_group_t *group) {
    uint32_t channels = 0;
    for (int i = 0; i < group->count; ++i)
        SoundAddCommand (.type = sound_command_fx_prepare, .data.fx_prepare.sound = group->sounds[i]);
    SoundFXPlayPrepared ();
    return channels;
}

void SoundFXPlayPrepared () {SoundAddCommand (.type = sound_command_fx_play_prepared);}

void SoundMusicPlay (const sound_music_t *new_music) {
    assert (new_music->count <= MUSIC_CHANNELS);
    SoundAddCommand (.type = sound_command_music_new, .data.music_new.music = new_music);
}

void SoundStopAll () {
    SoundMusicStop ();
    SoundFXStop ();
    // for (int c = 0; c < CHANNELS; ++c) {
    //     sound.channels[c].sound.waveform = sound_waveform_none;
    // }
}

static inline float CosineInterpolate (float d) { return (1.f - cos_turns(d/2)) / 2.f; }
static inline int16_t Lerpi16 (int16_t from, int16_t to, float ratio) { return (1-ratio)*from + to*ratio;}

static inline float PolyBLEP (float t, float frequency) {
    float dt = frequency / SAMPLING_RATE;
    if (t < dt) {
        t /= dt;
        return t + t - t*t - 1;
    }
    else if (t > 1 - dt) {
        t = (t - 1) / dt;
        return t*t +t + t + 1;
    }
    else return 0;
}

static inline float CreateNextSample () {
    static float noise_channels[SOUND_CHANNELS];
    float value_music = 0;
    float value_fx = 0;
    float *value = &value_music;
    for (int c = (sound.music.state != music_state_playing ? FX_CHANNELS_FIRST : MUSIC_CHANNELS_FIRST); c < SOUND_CHANNELS; ++c) {
        if (c > MUSIC_CHANNELS_LAST) value = &value_fx;
        auto channel = &sound.channels[c];
        auto sound = &channel->sound;
        float sample = 0;
        float envelope = 1;
        if (!(sound->waveform == sound_waveform_none || sound->waveform == sound_waveform_preparing)) {
            channel->t++;
            if (sound->waveform != sound_waveform_silence) {
                int16_t freq = Lerpi16(sound->frequency, sound->frequency + sound->sweep, (float)channel->t / sound->duration);
                if (sound->vibrato.vibrations_per_hundred_seconds) {
                    auto vibrato_range = sound->vibrato.frequency_range;
                    int16_t vibrato = sin_turns (channel->vibrato_d) * vibrato_range;
                    freq += vibrato;
                    channel->vibrato_d += sound->vibrato.vibrations_per_hundred_seconds / (float)(SAMPLING_RATE * 100);
                }
                // At this point, due to sweep and vibrato, freq may be negative. Turns out that's totally fine and allows for some nice effects!
                int prevd = channel->d;
                float d_change = (float)freq / SAMPLING_RATE;
                channel->d += d_change;
                switch (sound->waveform) {
                    case sound_waveform_sine: sample = sin_turns (channel->d); break;
                    case sound_waveform_triangle: {
                        float a = (channel->d - (int)channel->d);
                        if (a > .5) a = 1 - a;
                        sample = a * 4 - 1;
                    } break;
                    case sound_waveform_saw: {
                        sample = (channel->d - (int)channel->d) * 2 - 1;
                        sample -= PolyBLEP (channel->d - (int)channel->d, freq);
                    } break;
                    case sound_waveform_pulse: {
                        float pos = channel->d - (int)channel->d;
                        // Duty cycle 0 = 50%. 127 = 98%, -128 = 99%
                        // Duty cycle as int8, loops with overflow. Convert to float. Divide by 129 means computed duty cycle is never 0 (avoid pop when temporarily hitting 0) and 127 == -127 for perfect looping through -128
                        int8_t duty_cycle1 = (sound->square_duty_cycle + (int32_t)channel->t * sound->square_duty_cycle_sweep / (int32_t)sound->duration);
                        float duty_cycle = absf(duty_cycle1) / 257.f + .5f;
                        // if (duty_cycle < 0.5) duty_cycle = 1 - duty_cycle;
                        assert (duty_cycle >= 0.5 && duty_cycle < 1);
                        // sample = -2 * duty_cycle;
                        // if (pos < duty_cycle) sample += 2;
                        sample = pos >= duty_cycle ? -1 : 1;
                        const auto BLEP0 = PolyBLEP (pos, freq);
                        sample += BLEP0;
                        float pos2 = pos + 1 - duty_cycle;
                        pos2 -= (int)pos2;
                        const auto BLEP1 = PolyBLEP (pos2, freq);
                        sample -= BLEP1;
                    } break;
                    case sound_waveform_noise: {
                        channel->d += 3 * d_change;
                        if (prevd != (int)channel->d) {
                            noise_channels[c] = rand() / (float)RAND_MAX * 2 - 1;
                        }
                        sample = noise_channels[c];
                    } break;
                    case sound_waveform_none: case sound_waveform_preparing: case sound_waveform_silence: unreachable(); break;
                }
                // assert (sample >=-1 && sample <= 1);
                
                envelope = 1;
                switch (channel->ADSR_state) {
                    case 0: {
                        if (channel->t < sound->ADSR.attack) {
                            envelope = (float)channel->t / sound->ADSR.attack * sound->ADSR.peak;
                            break;
                        }
                        channel->ADSR_state = 1;
                    }
                    case 1: {
                        int32_t t = channel->t - sound->ADSR.attack;
                        if (t < sound->ADSR.decay) {
                            float ratio = (float)t / sound->ADSR.decay;
                            envelope = (1 - ratio) * sound->ADSR.peak + ratio * sound->ADSR.sustain;
                            break;
                        }
                        channel->ADSR_state = 2;
                    }
                    case 2: {
                        int32_t t = sound->duration - channel->t;
                        if (t >= sound->ADSR.release) {
                            envelope = sound->ADSR.sustain;
                            break;
                        }
                        channel->ADSR_state = 3;
                    }
                    case 3: {
                        int32_t t = channel->t + (sound->ADSR.release - sound->duration);
                        if (t <= 0) envelope = 0;
                        else envelope = (1.f - (float)t / sound->ADSR.release) * sound->ADSR.sustain;
                        break;
                    }
                }
                sample *= envelope;
                // assert (sample >=-1 && sample <= 1);
                *value += sample;
            }
            
            if (channel->t >= sound->duration){
                if (sound->next) {
                    auto tempd = channel->d;
                    auto tempvd = channel->vibrato_d;
                    auto tempstuff = channel->u;
                    *channel = (typeof(*channel)){.sound = *sound->next};
                    if (envelope > 0) {
                        channel->d = tempd;
                        channel->vibrato_d = tempvd;
                        channel->u = tempstuff;
                    }
                }
                else sound->waveform = sound_waveform_none;
            }
        }
    }
    // if (fabs (value) > 1) {
    //     static int clips = 0;
    //     LOG ("Clip[%d]", ++clips);
    // }

    return ((value_music * sound.music.volume) + (value_fx * sound.fx.volume)) * sound.master_volume;
}

static inline void RefillSampleBuffer () {
    SoundExecuteCommands ();

    if (sound_extern_data.prepared_sounds_ready) {
        for (int c = 0; c < SOUND_CHANNELS; ++c) {
            if (sound.channels[c].sound.waveform == sound_waveform_preparing) {
                assert (sound.channels[c].sound.next);
                sound.channels[c] = (typeof(sound.channels[c])){.sound = *sound.channels[c].sound.next};
            }
        }
        sound_extern_data.prepared_sounds_ready = false;
    }

    sample_buffer_swap = !sample_buffer_swap;
    static float compressor_level = 1;

    // Fill used-up buffer with new, uncompressed samples
    float peak = 1;
    for (int i = 0; i < sample_buffer_size; ++i) {
        float sample = CreateNextSample();
        float sampleabs = fabs (sample);
        if (sampleabs > peak) peak = sampleabs;
        sample_buffer[!sample_buffer_swap].samples[i] = sample;
    }
    if (peak > 1) peak += 0.00001f; // Tiny bit of extra for floating point inaccuracy
    else peak = 1; // Peak lower than 1, set to 1
    sample_buffer[!sample_buffer_swap].peak = peak;

    // Compress other buffer
    // Starting from compression level previous frame ended with, lerp to peak of (greater of current or next frame).
    float compressor_target = MAX (sample_buffer[sample_buffer_swap].peak, sample_buffer[!sample_buffer_swap].peak);
    for (int i = 0; i < sample_buffer_size; ++i) {
        float d = (float)i / sample_buffer_size;
        float compression = (1-d)*compressor_level + d*compressor_target;
        sample_buffer[sample_buffer_swap].samples[i] /= compression;
        assert (fabs (sample_buffer[sample_buffer_swap].samples[i]) <= 1);
    }
    compressor_level = compressor_target;
}