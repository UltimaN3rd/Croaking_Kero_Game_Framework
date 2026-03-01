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
#include "framework.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
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

typedef struct {
    sound_channel_t channels[SOUND_CHANNELS];
    struct {
        float volume;
    } fx;
    float master_volume;
    struct {
        const sound_music_t *new_source;
        float volume;
        enum {music_state_pause, music_state_playing} state;
        sound_music_t source;
    } music;
} sound_internal_t;

static sound_internal_t sound = {
    .music.volume = 1,
    .fx.volume = 1,
    .master_volume = .5,
};

static struct {
    uint8_t executed, filled; // when filled < executed, execute until overflow, then check again.
    // SIZE must be 256 so the overflow logic works
    #define COMMAND_BUFFER_SIZE 256
    struct {
        enum { sound_command_fx_stop, sound_command_fx_play, sound_command_fx_prepare, sound_command_fx_play_prepared, sound_command_fx_volume_set, sound_command_music_new, sound_command_music_pause, sound_command_music_resume, sound_command_music_volume_set } type;
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
                sound.channels[selected_channel] = (typeof(sound.channels[selected_channel])){
                    .sound = {
                        .waveform = sound_waveform_preparing,
                        .duration = UINT32_MAX,
                        .next = c.data.fx_prepare.sound,
                    }
                };
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

static int32_t sampling_rate = 48000;
static int32_t period_size = 
#ifdef __linux__
(sampling_rate / 200);
#elifdef WIN32
256;
#elifdef __APPLE__
240;
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

ADSR_t ADSRf_to_ADSR (ADSRf_t in) { return (ADSR_t){.peak = in.peak, .attack = in.attack * sampling_rate, .decay = in.decay * sampling_rate, .sustain = in.sustain, .release = in.release * sampling_rate}; }

sound_t SoundGenerate_(SoundFXPlay_args args) {
    assert (args.waveform);
    if (args.waveform == sound_waveform_none) return (sound_t){};
    args.ADSR.peak *= args.volume;
    args.ADSR.sustain *= args.volume;
    uint32_t duration = args.duration_samples;
    if (duration == 0) duration = args.duration_seconds * sampling_rate;
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
    float dt = frequency / sampling_rate;
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
                    channel->vibrato_d += sound->vibrato.vibrations_per_hundred_seconds / (float)(sampling_rate * 100);
                }
                // At this point, due to sweep and vibrato, freq may be negative. Turns out that's totally fine and allows for some nice effects!
                int prevd = channel->d;
                float d_change = (float)freq / sampling_rate;
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










#ifdef __linux__

// pulseaudio simple

#include <pulse/simple.h>
#include <pulse/error.h>

void *Sound (void *data_void) {
    sample_buffer_size = PERIOD_SIZE;
    RefillSampleBuffer ();

    pa_simple *simple = NULL;
    int pulse_error;
    { simple = pa_simple_new (NULL, GAME_TITLE, PA_STREAM_PLAYBACK, NULL, "playback", &(pa_sample_spec){.format = PA_SAMPLE_S16LE, .rate = SAMPLING_RATE, .channels = 1}, NULL, &(pa_buffer_attr){.maxlength = PERIOD_SIZE * 8, .tlength = PERIOD_SIZE * 4, .prebuf = PERIOD_SIZE * 2, .minreq = -1, .fragsize = PERIOD_SIZE * 2}, &pulse_error); if (simple == NULL) { LOG ("Failed to initialize PulseAudio [%s]", pa_strerror (pulse_error)); return NULL; }}

    while (!sound_extern_data.quit) {
        RefillSampleBuffer ();
        int buffer_index = 0;
        int16_t buffer[PERIOD_SIZE*sizeof(int16_t)];

        repeat (PERIOD_SIZE) buffer[buffer_index] = sample_buffer[sample_buffer_swap].samples[buffer_index] * INT16_MAX, buffer_index++;

        { auto result = pa_simple_write (simple, buffer, PERIOD_SIZE * 2, &pulse_error); if (result != 0) { LOG ("PulseAudio failed to write buffer [%s]", pa_strerror (pulse_error)); return NULL; } }
    }

    LOG ("Render thread exiting normally");

    return NULL;
}










#elifdef WIN32 // __linux__







#include <initguid.h>
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <avrt.h> // AvSetMmThreadCharacteristics()
#include <Functiondiscoverykeys_devpkey.h> // PKEY_Device_FriendlyName
#include <tchar.h>

static bool restart_audio = true;

// MMNoitifcationClient stuff to auto-switch when default device changes

static STDMETHODIMP MMNotificationClient_QueryInterface(IMMNotificationClient *self, REFIID riid, void **ppvObject) {
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IMMNotificationClient)) {
        *ppvObject = self;
        return S_OK;
    }
    else {
       *ppvObject = NULL;
        return E_NOINTERFACE;
    }
}

static ULONG MMNotificationClient_AddRef(IMMNotificationClient *self) { return 1; } // Just don't count the references
static ULONG MMNotificationClient_Release(IMMNotificationClient *self) { return 0; }
static STDMETHODIMP MMNotificationClient_OnDeviceStateChanged(IMMNotificationClient *self, LPCWSTR wid, DWORD state) { LOG ("WASAPI device state changed"); return S_OK; }
static STDMETHODIMP MMNotificationClient_OnDeviceAdded(IMMNotificationClient *self, LPCWSTR wid) { LOG ("WASAPI device added"); return S_OK; }
static STDMETHODIMP MMNotificationClient_OnDeviceRemoved(IMMNotificationClient *self, LPCWSTR wid) { LOG ("WASAPI device removed"); return S_OK; }
static STDMETHODIMP MMNotificationClient_OnPropertyValueChanged(IMMNotificationClient *self, LPCWSTR wid, const PROPERTYKEY key) { LOG ("WASAPI property value changed"); return S_OK; }

static STDMETHODIMP MMNotificationClient_OnDefaultDeviceChange(IMMNotificationClient *self, EDataFlow flow, ERole role, LPCWSTR wid) {
    LOG ("WASAPI default device changed");
    restart_audio = true;
    return S_OK;
}

static IMMNotificationClient notification_client = {
    .lpVtbl = &(struct IMMNotificationClientVtbl){
        MMNotificationClient_QueryInterface,
        MMNotificationClient_AddRef,
        MMNotificationClient_Release,
        MMNotificationClient_OnDeviceStateChanged,
        MMNotificationClient_OnDeviceAdded,
        MMNotificationClient_OnDeviceRemoved,
        MMNotificationClient_OnDefaultDeviceChange,
        MMNotificationClient_OnPropertyValueChanged,
    }
};

const char *HResultToStr (HRESULT result) {
    static char wasapi_error_buffer[512];
    DWORD len;  // Number of chars returned.
    len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, result, 0, wasapi_error_buffer, 512, NULL);
    if (len == 0) {
        HINSTANCE hInst = LoadLibraryA("Ntdsbmsg.dll");
        if (hInst == NULL)
            snprintf (wasapi_error_buffer, sizeof (wasapi_error_buffer), "Cannot convert error to string: Failed to load Ntdsbmsg.dll");
        else {
            len = FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS, hInst, result, 0, wasapi_error_buffer, 512, NULL);
            if (len == 0) snprintf (wasapi_error_buffer, sizeof (wasapi_error_buffer), "HRESULT error message not found");
            FreeLibrary( hInst );
        }
    }
    return wasapi_error_buffer;
}

#define DO_OR_QUIT(function_call__, error_message__) do { auto result = function_call__; assert (result == S_OK); if (result != S_OK) { LOG (error_message__ " [%ld] [%s]", result, HResultToStr(result)); return NULL; } } while (0)
#define DO_OR_CONTINUE(function_call__, error_message__) do { auto result = function_call__; if (result != S_OK) { LOG (error_message__ " [%ld] [%s]", result, HResultToStr(result)); } } while (0)
#define QUIT_IF_NULL(variable__, error_message__) do { auto __var__ = (variable__); assert (__var__); if (__var__ == NULL) { LOG (error_message__); return NULL; } } while (0)
#define IF_NULL_RESTART(variable__, message__) if (variable__ == NULL) { LOG (message__); restart_audio = true; continue; }
#define BREAK_ON_FAIL(code__, message__) { auto result = code__; assert (!FAILED(result)); if (FAILED(result)) { LOG (message__); break; } }
#define DO_OR_RESTART(function_call__, error_message__) ({ auto result = function_call__; if (result != S_OK) { LOG (error_message__ " [%ld] [%s]", result, HResultToStr(result)); restart_audio = true; continue; } })

#pragma push_macro ("TYPE__")
#pragma push_macro ("FUNCNAME__")
#undef TYPE__
#define TYPE__ int32_t
#undef FUNCNAME__
#define FUNCNAME__ SoundLoop_int
#include "wasapi_sound_func.c"
#undef TYPE__
#define TYPE__ float
#undef FUNCNAME__
#define FUNCNAME__ SoundLoop_float
#include "wasapi_sound_func.c"
#pragma pop_macro ("FUNCNAME__")
#pragma pop_macro ("TYPE__")

void *Sound (void *data_void) {
    DO_OR_QUIT (CoInitializeEx(NULL, 0), "Sound thread failed to CoInitializeEx");
    defer { CoUninitialize(); }

	{ DWORD task_index = 0; if (AvSetMmThreadCharacteristics(TEXT("Pro Audio"), &task_index) == 0) { LOG ("WASAPI failed to set thread characteristics to \"Pro Audio\""); } }

	IMMDeviceEnumerator *device_enumerator = NULL;
	DO_OR_QUIT (CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator, (void**)&device_enumerator), "WASAPI failed to create Device Enumerator");
    QUIT_IF_NULL (device_enumerator, "WASAPI failed to create Device Enumerator, even though CoCreateInstance returned S_OK");
	defer { device_enumerator->lpVtbl->Release(device_enumerator); }

    bool multiple_restart_attempts = false;
    while (restart_audio && !sound_extern_data.quit) {
        if (multiple_restart_attempts) Sleep (1000);
        multiple_restart_attempts = true;
        LOG ("Initializing WASAPI device");

        device_enumerator->lpVtbl->RegisterEndpointNotificationCallback (device_enumerator, &notification_client);
        defer { device_enumerator->lpVtbl->UnregisterEndpointNotificationCallback (device_enumerator, &notification_client); }

        IMMDevice *default_device = NULL;
        DO_OR_RESTART (device_enumerator->lpVtbl->GetDefaultAudioEndpoint(device_enumerator, eRender, eConsole, &default_device), "WASAPI failed to get default audio endpoint");
        IF_NULL_RESTART (default_device, "WASAPI failed to create get default audio endpoint, even though GetDefaultAudioEndpoint returned S_OK");

        do { // Print default audio device friendly name
            LPWSTR id = NULL;
            BREAK_ON_FAIL (default_device->lpVtbl->GetId (default_device, &id), "Couldn't get default device ID"); 

            IPropertyStore *properties = NULL;
            BREAK_ON_FAIL (default_device->lpVtbl->OpenPropertyStore (default_device, STGM_READ, &properties), "Couldn't open property store of default device");
            defer { properties->lpVtbl->Release (properties); }

            PROPVARIANT property_name;
            PropVariantInit (&property_name);
            BREAK_ON_FAIL (properties->lpVtbl->GetValue (properties, &PKEY_Device_FriendlyName, &property_name), "Failed to get default audio device name");
            defer { PropVariantClear (&property_name); }

            if (property_name.vt != VT_EMPTY) LOG ("Default audio device selected: [%S]", property_name.pwszVal);
            else LOG ("Default audio device name blank?");
        } while (0);

        IAudioClient3 *client = NULL;
        DO_OR_RESTART (default_device->lpVtbl->Activate(default_device, &IID_IAudioClient3, CLSCTX_ALL, NULL, (void**)&client), "WASAPI failed to activate audio client");
        IF_NULL_RESTART (client, "WASAPI failed to activate audio client, even though Activate returned S_OK");
        defer { client->lpVtbl->Release(client); }

        default_device->lpVtbl->Release(default_device);

        AudioClientProperties audio_properties = {
            .cbSize = sizeof(AudioClientProperties),
            .bIsOffload = FALSE,
            .eCategory = AudioCategory_GameEffects,
        };
        DO_OR_RESTART (client->lpVtbl->SetClientProperties(client, &audio_properties), "WASAPI failed to set client properties");

        WAVEFORMATEXTENSIBLE wave_format = {
            .Format = {
                .wFormatTag = WAVE_FORMAT_PCM,
                .nChannels = 2,
                .nSamplesPerSec = sampling_rate,
                .wBitsPerSample = 16,
            },
        };
        wave_format.Format.nBlockAlign = wave_format.Format.nChannels * wave_format.Format.wBitsPerSample / 8;
        wave_format.Format.nAvgBytesPerSec = wave_format.Format.nSamplesPerSec * wave_format.Format.nBlockAlign;

        {
            WAVEFORMATEX *wave_format_closest;
            HRESULT result = client->lpVtbl->IsFormatSupported (client, AUDCLNT_SHAREMODE_SHARED, &wave_format.Format, &wave_format_closest);
            if (result == S_OK) {
                LOG ("Default audio format supported");
            }
            else if (result == S_FALSE && wave_format_closest != NULL) {
                wave_format = *(WAVEFORMATEXTENSIBLE*)wave_format_closest;
                LOG ("Default audio format unsupported. Using this instead: Channels[%d] Bits per sample[%d] Sampling rate[%lu] Extensible[%s] Valid bits per sample[%d] Format: [", wave_format.Format.nChannels, wave_format.Format.wBitsPerSample, wave_format.Format.nSamplesPerSec, wave_format_closest->wFormatTag == WAVE_FORMAT_EXTENSIBLE ? "true" : "false", wave_format.Samples.wValidBitsPerSample);
                sampling_rate = wave_format.Format.nSamplesPerSec;
                if (IsEqualGUID(&wave_format.SubFormat, &KSDATAFORMAT_SUBTYPE_PCM)) LOG ("KSDATAFORMAT_SUBTYPE_PCM]");
                else if (IsEqualGUID(&wave_format.SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) LOG ("KSDATAFORMAT_SUBTYPE_IEEE_FLOAT]");
                else if (IsEqualGUID(&wave_format.SubFormat, &KSDATAFORMAT_SUBTYPE_DRM)) LOG ("KSDATAFORMAT_SUBTYPE_DRM]");
                else if (IsEqualGUID(&wave_format.SubFormat, &KSDATAFORMAT_SUBTYPE_ALAW)) LOG ("KSDATAFORMAT_SUBTYPE_ALAW]");
                else if (IsEqualGUID(&wave_format.SubFormat, &KSDATAFORMAT_SUBTYPE_MULAW)) LOG ("KSDATAFORMAT_SUBTYPE_MULAW]");
                else if (IsEqualGUID(&wave_format.SubFormat, &KSDATAFORMAT_SUBTYPE_ADPCM)) LOG ("KSDATAFORMAT_SUBTYPE_ADPCM]");
                else { LOG ("UNKNOWN]"); return NULL; }
            }
            else { LOG ("No audio format supported"); return NULL; }
        }

        uint32_t period_default = 0, period_fundamental = 0, period_min = 0, period_max = 0;
        DO_OR_RESTART (client->lpVtbl->GetSharedModeEnginePeriod (client, &wave_format.Format, &period_default, &period_fundamental, &period_min, &period_max), "WASAPI failed to get shared mode engine period");
        // Calculate closest to 5ms period
        uint32_t frames_per_period = period_min;
        const uint32_t frames_for_5ms = sampling_rate / 200;
        if (period_min < frames_for_5ms) {
            const uint32_t blobs = (frames_for_5ms - period_min) / period_fundamental;
            frames_per_period += blobs * period_fundamental;
            if (frames_per_period < frames_for_5ms) frames_per_period += period_fundamental;
        }
        LOG ("Device min period [%u] requested period [%u] samples [%.2fms]", period_min, frames_per_period, 1000.f * frames_per_period / wave_format.Format.nSamplesPerSec);
        DO_OR_RESTART (client->lpVtbl->InitializeSharedAudioStream(client, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, frames_per_period, &wave_format.Format, NULL), "WASAPI failed to initialize shared audio stream");

        HANDLE event_handle = CreateEvent (NULL, FALSE, FALSE, NULL);
        IF_NULL_RESTART (event_handle, "Failed to create event handle");
        DO_OR_RESTART (client->lpVtbl->SetEventHandle (client, event_handle), "WASAPI failed to set event handle");

        IAudioRenderClient *render = NULL;
        DO_OR_RESTART (client->lpVtbl->GetService(client, &IID_IAudioRenderClient, (void**)&render), "WASAPI failed to get AudioRenderClient service");
        IF_NULL_RESTART (render, "Failed to get AudioRenderClient even though GetService returned S_OK");
        defer { render->lpVtbl->Release(render); }

        DO_OR_RESTART (client->lpVtbl->Start(client), "WASAPI failed to start audio client");

        uint32_t format = wave_format.Format.wFormatTag;
        if (format == WAVE_FORMAT_EXTENSIBLE) {
            if (IsEqualGUID(&wave_format.SubFormat, &KSDATAFORMAT_SUBTYPE_PCM))
                format = WAVE_FORMAT_PCM;
            else if (IsEqualGUID(&wave_format.SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))
                format = WAVE_FORMAT_IEEE_FLOAT;
            else {
                format = 0;
                LOG ("Unsupported wave format: [%lx]", wave_format.SubFormat.Data1);
            }
        }
        else {
            wave_format.Samples.wValidBitsPerSample = wave_format.Format.wBitsPerSample;
        }

        restart_audio = multiple_restart_attempts = false;

        switch (format) {
            case WAVE_FORMAT_PCM: {
                LOG ("Entering SoundLoop_int");
                SoundLoop_int (wave_format, event_handle, render, frames_per_period);
            } break;
            case WAVE_FORMAT_IEEE_FLOAT: {
                LOG ("Entering SoundLoop_float");
                SoundLoop_float (wave_format, event_handle, render, frames_per_period);
            } break;
            // Handle the other formats at some point?
            default: LOG ("Not entering a sound loop - unsupported format"); break;
        }
    }

    LOG ("Sound thread exiting");

	return NULL;
}







#elifdef __APPLE__  // __linux__








#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>

static AudioUnit toneUnit;
OSStatus myAudioObjectPropertyListenerProc (AudioObjectID inObjectID, UInt32 inNumberAddresses, const AudioObjectPropertyAddress *inAddresses,void* __nullable inClientData);

OSStatus SoundCallback (void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData) {
    SInt16 *left = (SInt16*)ioData->mBuffers[0].mData;

    if (inNumberFrames != sample_buffer_size) {
        LOG ("Reset audio buffer size!");
        UInt32 period_size_in = sample_buffer_size;
        OSErr err = AudioUnitSetProperty(toneUnit, kAudioDevicePropertyBufferFrameSize,
                kAudioUnitScope_Input, 0, &period_size_in, sizeof(period_size_in));
        if (err) LOG("Error setting stream format: %d", err);
    }

    for (UInt32 frame = 0; frame < inNumberFrames; ++frame) {
        *(left++) = sample_buffer[sample_buffer_swap].samples[frame] * INT16_MAX;
    }

    RefillSampleBuffer();

    return noErr;
}

// OSStatus myAudioObjectPropertyListenerProc (AudioObjectID inObjectID, UInt32 inNumberAddresses, const AudioObjectPropertyAddress *inAddresses,void* __nullable inClientData) {
//     LOG ("Hello!");
//     for (int i = 0; i < inNumberAddresses; ++i) {
//         AudioObjectPropertyAddress currentAddress = inAddresses[i];

//         switch(currentAddress.mSelector) {
//             case kAudioDevicePropertyBufferFrameSize:
//             case kAudioDevicePropertyDeviceIsAlive:
//             case kAudioHardwarePropertyDevices:
//             {
//                 LOG ("Frame size reset!");
//                 UInt32 period_size_in = sample_buffer_size;
//                 OSErr err = AudioUnitSetProperty(toneUnit, kAudioDevicePropertyBufferFrameSize,
//                         kAudioUnitScope_Input, 0, &period_size_in, sizeof(period_size_in));
//                 if (err) LOG("Error setting stream format: %d", err);
//                 break;
//             }
//         }
//     }
//     // LOG ("Audio component count: %u", AudioComponentCount (&acd));
//     // AudioComponentDescription desc;
//     // AudioComponentGetDescription(output, &desc);
//     // LOG ("desc: %u %u %u %u %u", desc.componentFlags, desc.componentFlagsMask, desc.componentManufacturer, desc.componentSubType, desc.componentType);
//     // AudioComponentInstanceDispose (toneUnit);
//     // InitializeAudio ();
//     return noErr;
// }

#define DO_OR_QUIT(__function__, __error_message__) { OSErr err = __function__; if (err != 0) { LOG (__error_message__ "[%d]", err); return NULL; } }

void *Sound (void* args) {
    sample_buffer_size = PERIOD_SIZE;

    RefillSampleBuffer();

    const AudioComponentDescription acd = {
        .componentType = kAudioUnitType_Output,
        .componentSubType = kAudioUnitSubType_DefaultOutput,
        .componentManufacturer = kAudioUnitManufacturer_Apple,
    };
    AudioComponent output;

    { output = AudioComponentFindNext(NULL, &acd); if (!output) { LOG("Can't find default output"); return NULL; } }
    DO_OR_QUIT (AudioComponentInstanceNew(output, &toneUnit), "Error creating audio unit");
    AURenderCallbackStruct input = { .inputProc = SoundCallback };
    DO_OR_QUIT (AudioUnitSetProperty(toneUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &input, sizeof(input)), "Error setting render callback");

    AudioStreamBasicDescription asbd = {
        .mFormatID = kAudioFormatLinearPCM,
        .mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked | kAudioFormatFlagIsNonInterleaved,
        .mSampleRate = sampling_rate,
        .mBitsPerChannel = 16,
        .mChannelsPerFrame = 1,
        .mFramesPerPacket = 1,
        .mBytesPerFrame = 2,
        .mBytesPerPacket = 2,
    };
    DO_OR_QUIT (AudioUnitSetProperty(toneUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &asbd, sizeof(asbd)), "Error setting stream format");

    UInt32 period_size_in = sample_buffer_size;
    DO_OR_QUIT (AudioUnitSetProperty(toneUnit, kAudioDevicePropertyBufferFrameSize, kAudioUnitScope_Input, 0, &period_size_in, sizeof(period_size_in)), "Error setting stream format");
    DO_OR_QUIT (AudioUnitInitialize(toneUnit), "Error initializing unit");
    DO_OR_QUIT (AudioOutputUnitStart(toneUnit), "Error starting unit");

    return NULL;
}





#endif // __APPLE__
