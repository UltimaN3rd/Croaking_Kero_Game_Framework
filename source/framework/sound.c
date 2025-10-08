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
#define FX_CHANNELS 16
#define FX_CHANNELS_LAST (MUSIC_CHANNELS + FX_CHANNELS - 1)
#define CHANNELS (MUSIC_CHANNELS + FX_CHANNELS)
static_assert (SOUND_CHANNELS == CHANNELS);

sound_extern_t sound_extern_data;

sound_internal_t sound = {
    .music.volume = 1,
    .fx.volume = 1,
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
        if (sound.channels[i].sample == NULL) {
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
                    sound.channels[i].sample = NULL;
            } break;
            case sound_command_fx_play: {
                int selected_channel = SelectFXChannel ();
                if (selected_channel == -1) break;
                sound.channels[selected_channel] = c.data.fx_play.sound;
                sound.channels[selected_channel].d = 0;
            } break;
            case sound_command_fx_prepare: {
                int selected_channel = SelectFXChannel ();
                if (selected_channel == -1) break;
                sound.channels[selected_channel].sample = &sound_sample_preparing;
                sound.channels[selected_channel].duration = UINT32_MAX;
                sound.channels[selected_channel].next = c.data.fx_prepare.sound;
                sound.channels[selected_channel].d = 0;
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
                    sound.channels[c].sample = &sound_sample_silence;
                int channel_count = sound.music.new_source->count;
                assert (channel_count <= MUSIC_CHANNELS); if (channel_count > MUSIC_CHANNELS) { LOG ("New music requested has too many channels [%d] > [%d]", channel_count, MUSIC_CHANNELS); channel_count = MUSIC_CHANNELS; }
                for (int c = 0; c < channel_count; ++c)
                    sound.channels[c] = *sound.music.new_source->sounds[c];
            } break;
            case sound_command_music_pause: {
                sound.music.state = music_state_pause;
            } break;
            case sound_command_music_resume: {
                sound.music.state = music_state_playing;
            } break;
            case sound_command_music_volume_set: {
                assert (c.data.music_volume_set.volume >= 0); if (c.data.music_volume_set.volume < 0) c.data.music_volume_set.volume = 0;
                assert (c.data.music_volume_set.volume <= 1); if (c.data.music_volume_set.volume > 1) c.data.music_volume_set.volume = 1;
                sound.music.volume = c.data.music_volume_set.volume;
            } break;
        }
    }
    assert (command_buffer.executed == filled);
}

void SoundMusicStop () {
    SoundAddCommand (.type = sound_command_music_pause);
}
void SoundMusicResume () {
    SoundAddCommand (.type = sound_command_music_resume);
}

void SoundMusicSetVolume (float volume_0_to_1) {
    assert (volume_0_to_1 >= 0); if (volume_0_to_1 < 0) volume_0_to_1 = 0;
    assert (volume_0_to_1 <= 1); if (volume_0_to_1 > 1) volume_0_to_1 = 1;
    SoundAddCommand (.type = sound_command_music_volume_set, .data.music_volume_set.volume = volume_0_to_1);
}
float SoundMusicGetVolume () { return sound.music.volume; }

void SoundFXStop () {
    SoundAddCommand (.type = sound_command_fx_stop);
}
void SoundFXSetVolume (float volume_0_to_1) {
    assert (volume_0_to_1 >= 0); if (volume_0_to_1 < 0) volume_0_to_1 = 0;
    assert (volume_0_to_1 <= 1); if (volume_0_to_1 > 1) volume_0_to_1 = 1;
    SoundAddCommand (.type = sound_command_fx_volume_set, .data.fx_volume_set.volume = volume_0_to_1);
}
float SoundFXGetVolume () { return sound.fx.volume; }

#define SAMPLING_RATE 48000
#ifdef __linux__
#define PERIOD_SIZE (SAMPLING_RATE / 200)
#elifdef WIN32
#define PERIOD_SIZE 256
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
    assert (args.sample);
    if (args.sample == NULL) return (sound_t){};
    if (args.frequency_end == UINT16_MAX) {
        args.frequency_end = args.frequency;
    }
    args.ADSR.peak *= args.volume;
    args.ADSR.sustain *= args.volume;
    sound_t sound = {
        .sample = args.sample,
        .duration = args.duration * SAMPLING_RATE,
        .next = NULL,
        .ADSR = ADSRf_to_ADSR(args.ADSR),
        .frequency_begin = args.frequency,
        .frequency_end = args.frequency_end,
    };
    int duration = sound.ADSR.attack + sound.ADSR.decay + sound.ADSR.release;
    if (sound.duration < duration) { // There isn't enough time for the full envelope, even with 0 sustain. Get ratio of envelope and apply to the reduced duration
        float total = args.ADSR.attack + args.ADSR.decay + args.ADSR.release;
        sound.ADSR.attack = (args.ADSR.attack / total) * sound.duration;
        sound.ADSR.decay = (args.ADSR.decay / total) * sound.duration;
        sound.ADSR.release = sound.duration - sound.ADSR.attack - sound.ADSR.decay;
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
    // assert (!sound_extern_data.prepared_sounds_ready);
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
    //     sound.channels[c].sample = NULL;
    // }
}

static inline float CosineInterpolate (float d) { return (1.f - cos_turns(d/2)) / 2.f; }
static inline int16_t Lerpi16 (int16_t from, int16_t to, float ratio) { return (1-ratio)*from + to*ratio;}





static inline float CreateNextSample () {
    static float noise_channels[CHANNELS];
    float value_music = 0;
    float value_fx = 0;
    float *value = &value_music;
    for (int c = (sound.music.state != music_state_playing ? FX_CHANNELS_FIRST : MUSIC_CHANNELS_FIRST); c < CHANNELS; ++c) {
        if (c > MUSIC_CHANNELS_LAST) value = &value_fx;
        auto channel = &sound.channels[c];
        float sample = 0;
        if (!(channel->sample == NULL || channel->sample == &sound_sample_preparing)) {
            channel->t++;
            if (channel->sample != &sound_sample_silence) {
                int16_t freq = Lerpi16(channel->frequency_begin, channel->frequency_end, (float)channel->t / channel->duration);
                int prevd = channel->d;
                channel->d += (float)freq / SAMPLING_RATE;
                if (channel->sample == &sound_sample_noise) {
                    if (prevd != (int)channel->d) {
                        noise_channels[c] = rand() / (float)RAND_MAX * 2 - 1;
                    }
                    sample = noise_channels[c];
                }
                else {
                    float picker = channel->d * channel->sample->count;
                    int pick0 = (int)picker % channel->sample->count;
                    float frac = picker - (int)picker;
                    int pick1 = (pick0 + 1) % channel->sample->count;
                    sample = (float)Lerpi16(channel->sample->samples[pick0], channel->sample->samples[pick1], frac) / INT16_MAX;
                }
                assert (sample >=-1 && sample <= 1);
                sample *= 0.8f; // Temporary attenuation solution. In future I'll do a lookahead compressor
                
                float envelope = 1;
                switch (channel->ADSR_state) {
                    case 0: {
                        if (channel->t <= channel->ADSR.attack) {
                            envelope = (float)channel->t / channel->ADSR.attack * channel->ADSR.peak;
                            break;
                        }
                        channel->ADSR_state = 1;
                    }
                    case 1: {
                        uint32_t t = channel->t - channel->ADSR.attack;
                        if (t <= channel->ADSR.decay) {
                            float ratio = (float)t / channel->ADSR.decay;
                            envelope = (1 - ratio) * channel->ADSR.peak + ratio * channel->ADSR.sustain;
                            break;
                        }
                        channel->ADSR_state = 2;
                    }
                    case 2: {
                        uint32_t t = channel->duration - channel->t;
                        if (t > channel->ADSR.release) {
                            envelope = channel->ADSR.sustain;
                            break;
                        }
                        channel->ADSR_state = 3;
                    }
                    case 3: {
                        uint32_t t = channel->t + (channel->ADSR.release - channel->duration);
                        envelope = (1.f - (float)t / channel->ADSR.release) * channel->ADSR.sustain;
                        break;
                    }
                }
                sample *= envelope;
                assert (sample >=-1 && sample <= 1);
                *value += sample;
            }
            
            if (channel->t >= channel->duration){
                if (channel->next) {
                    *channel = *channel->next;
                }
                else channel->sample = NULL;
            }
        }
    }
    // if (fabs (value) > 1) {
    //     static int clips = 0;
    //     LOG ("Clip[%d]", ++clips);
    // }

    return (value_music * sound.music.volume) + (value_fx * sound.fx.volume);
}

static inline void RefillSampleBuffer () {
    SoundExecuteCommands ();

    if (sound_extern_data.prepared_sounds_ready) {
        for (int c = 0; c < CHANNELS; ++c) {
            if (sound.channels[c].sample == &sound_sample_preparing) {
                assert (sound.channels[c].next);
                sound.channels[c] = *sound.channels[c].next;
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
    { simple = pa_simple_new (NULL, "Simple sound thing", PA_STREAM_PLAYBACK, NULL, "playback", &(pa_sample_spec){.format = PA_SAMPLE_S16LE, .rate = SAMPLING_RATE, .channels = 1}, NULL, &(pa_buffer_attr){.maxlength = PERIOD_SIZE * 8, .tlength = PERIOD_SIZE * 4, .prebuf = PERIOD_SIZE * 2, .minreq = -1, .fragsize = PERIOD_SIZE * 2}, &pulse_error); assert (simple); if (simple == NULL) { LOG ("Failed to initialize PulseAudio [%s]", pa_strerror (pulse_error)); return NULL; }}

    while (!sound_extern_data.quit) {
        RefillSampleBuffer ();
        int buffer_index = 0;
        int16_t buffer[PERIOD_SIZE*sizeof(int16_t)];

        repeat (PERIOD_SIZE) buffer[buffer_index] = sample_buffer[sample_buffer_swap].samples[buffer_index] * INT16_MAX, buffer_index++;

        { auto result = pa_simple_write (simple, buffer, PERIOD_SIZE * 2, &pulse_error); assert (result == 0); if (result != 0) { LOG ("PulseAudio failed to write buffer [%s]", pa_strerror (pulse_error)); return NULL; } }
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

#include <tchar.h>

void Sound_Loop_i8 (WAVEFORMATEXTENSIBLE wave_format, HANDLE event_handle, IAudioRenderClient *render, UINT32 frames_per_period);
void Sound_Loop_i16 (WAVEFORMATEXTENSIBLE wave_format, HANDLE event_handle, IAudioRenderClient *render, UINT32 frames_per_period);
void Sound_Loop_i32 (WAVEFORMATEXTENSIBLE wave_format, HANDLE event_handle, IAudioRenderClient *render, UINT32 frames_per_period);
void Sound_Loop_Float (WAVEFORMATEXTENSIBLE wave_format, HANDLE event_handle, IAudioRenderClient *render, UINT32 frames_per_period);
auto Sound_Loop_Default = Sound_Loop_i16;

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

#define DO_OR_QUIT(__function_call__, __error_message__) do { auto result = __function_call__; assert (result == S_OK); if (result != S_OK) { LOG (__error_message__ " [%ld] [%s]", result, HResultToStr(result)); return NULL; } } while (0)
#define DO_OR_CONTINUE(__function_call__, __error_message__) do { auto result = __function_call__; if (result != S_OK) { LOG (__error_message__ " [%ld] [%s]", result, HResultToStr(result)); } } while (0)
#define QUIT_IF_NULL(__variable__, __error_message__) do { auto __var__ = (__variable__); assert (__var__); if (__var__ == NULL) { LOG (__error_message__); return NULL; } } while (0)

void *Sound (void *data_void) {
    DO_OR_QUIT (CoInitializeEx(NULL, 0), "Sound thread failed to CoInitializeEx");
	IMMDeviceEnumerator *device_enumerator = NULL;
	DO_OR_QUIT (CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator, (void**)&device_enumerator), "WASAPI failed to create Device Enumerator");
    QUIT_IF_NULL (device_enumerator, "WASAPI failed to create Device Enumerator, even though CoCreateInstance returned S_OK");
	IMMDevice *default_device = NULL;
	DO_OR_QUIT (device_enumerator->lpVtbl->GetDefaultAudioEndpoint(device_enumerator, eRender, eConsole, &default_device); assert (result == S_OK), "WASAPI failed to get default audio endpoint");
    QUIT_IF_NULL (default_device, "WASAPI failed to create get default audio endpoint, even though GetDefaultAudioEndpoint returned S_OK");
	DO_OR_CONTINUE (device_enumerator->lpVtbl->Release(device_enumerator), "Device Enumerator Release() failed");

	IAudioClient3 *client = NULL;
	DO_OR_QUIT (default_device->lpVtbl->Activate(default_device, &IID_IAudioClient3, CLSCTX_ALL, NULL, (void**)&client), "WASAPI failed to activate audio client");
    QUIT_IF_NULL (client, "WASAPI failed to activate audio client, even though Activate returned S_OK");

	DO_OR_CONTINUE (default_device->lpVtbl->Release(default_device), "Audio Client Release() failed");

	AudioClientProperties audio_properties = {
		.cbSize = sizeof(AudioClientProperties),
		.bIsOffload = FALSE,
		.eCategory = AudioCategory_GameEffects,
	};
	DO_OR_QUIT (client->lpVtbl->SetClientProperties(client, &audio_properties), "WASAPI failed to set client properties");

	WAVEFORMATEXTENSIBLE wave_format = {
		.Format = {
			.wFormatTag = WAVE_FORMAT_PCM,
			.nChannels = 2,
			.nSamplesPerSec = SAMPLING_RATE,
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
            LOG ("Default audio format unsupported. Using this instead: Channels[%d] Bits per sample[%d] Extensible[%s] Valid bits per sample[%d] Format: [", wave_format.Format.nChannels, wave_format.Format.wBitsPerSample, wave_format_closest->wFormatTag == WAVE_FORMAT_EXTENSIBLE ? "true" : "false", wave_format.Samples.wValidBitsPerSample);
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

	UINT32 period_default = 0, period_fundamental = 0, period_min = 0, period_max = 0;
	UINT32 frames_per_period;
	DO_OR_QUIT (client->lpVtbl->GetSharedModeEnginePeriod (client, &wave_format.Format, &period_default, &period_fundamental, &period_min, &period_max), "WASAPI failed to get shared mode engine period");
    frames_per_period = period_min;
    if (frames_per_period < PERIOD_SIZE) { // Windows supports below 5ms? Wow! But we need 5ms or more to compress well.
        frames_per_period = (PERIOD_SIZE / period_fundamental) * period_fundamental;
        if (PERIOD_SIZE % period_fundamental != 0) frames_per_period += period_fundamental;
        if (frames_per_period > period_max) {
            LOG ("frames_per_period > period_max: %u > %u. Sound compression might be gnarly", frames_per_period, period_max);
            frames_per_period = period_max;
        }
    }
    sample_buffer_size = frames_per_period;
    assert (sample_buffer_size <= SAMPLE_BUFFER_SIZE); if (sample_buffer_size > SAMPLE_BUFFER_SIZE) { LOG ("sample_buffer_size[%d] > SAMPLE_BUFFER_SIZE[%d]", sample_buffer_size, SAMPLE_BUFFER_SIZE); }
	LOG ("Device min period [%u] requested period [%u] samples [%.2fms]", period_min, frames_per_period, 1000.f * frames_per_period / wave_format.Format.nSamplesPerSec);
	DO_OR_QUIT (client->lpVtbl->InitializeSharedAudioStream(client, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, frames_per_period, &wave_format.Format, NULL), "WASAPI failed to initialize shared audio stream");

	HANDLE event_handle = CreateEvent (NULL, FALSE, FALSE, NULL);
	QUIT_IF_NULL (event_handle, "Failed to create event handle");
	DO_OR_QUIT (client->lpVtbl->SetEventHandle (client, event_handle), "WASAPI failed to set event handle");

	IAudioRenderClient *render = NULL;
	// const GUID _IID_IAudioRenderClient = {0xf294acfc, 0x3146, 0x4483, {0xa7,0xbf, 0xad,0xdc,0xa7,0xc2,0x60,0xe2}};
	DO_OR_QUIT (client->lpVtbl->GetService(client, &IID_IAudioRenderClient, (void**)&render), "WASAPI failed to get AudioRenderClient service");
    QUIT_IF_NULL (render, "Failed to get AudioRenderClient even though GetService returned S_OK");

	{ DWORD task_index = 0; if (AvSetMmThreadCharacteristics(TEXT("Pro Audio"), &task_index) == 0) { LOG ("WASAPI failed to set thread characteristics to \"Pro Audio\""); } }

	DO_OR_QUIT (client->lpVtbl->Start(client), "WASAPI failed to start audio client");

	if (wave_format.Format.wFormatTag != WAVE_FORMAT_EXTENSIBLE) {
		if (wave_format.Format.wFormatTag != WAVE_FORMAT_PCM || wave_format.Format.nChannels != 2 || wave_format.Format.nSamplesPerSec != 48000 || wave_format.Format.wBitsPerSample != 16) {
			LOG ("Mismatch between default audio WAVEFORMATEX and the default audio output function. Please change the defaults back or make a custom function to output the correct samples.");
		}
		Sound_Loop_Default (wave_format, event_handle, render, frames_per_period);
	}
	else {
		if (IsEqualGUID(&wave_format.SubFormat, &KSDATAFORMAT_SUBTYPE_PCM)) {
			// Handle different bit depths and stuff
			if (wave_format.Format.wBitsPerSample == 8)
				Sound_Loop_i8 (wave_format, event_handle, render, frames_per_period);
			else if (wave_format.Format.wBitsPerSample == 16)
				Sound_Loop_i16 (wave_format, event_handle, render, frames_per_period);
			else if (wave_format.Format.wBitsPerSample == 32)
				Sound_Loop_i32 (wave_format, event_handle, render, frames_per_period);
		}
		else if (IsEqualGUID(&wave_format.SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) {
			// Do I need to handle any specifics here?
			Sound_Loop_Float (wave_format, event_handle, render, frames_per_period);
		}
		// Handle the other formats at some point?
		else {
			if (IsEqualGUID(&wave_format.SubFormat, &KSDATAFORMAT_SUBTYPE_DRM))
                LOG ("Audio format KSDATAFORMAT_SUBTYPE_DRM unsupported");
            else if (IsEqualGUID(&wave_format.SubFormat, &KSDATAFORMAT_SUBTYPE_ALAW))
                LOG ("Audio format KSDATAFORMAT_SUBTYPE_ALAW unsupported");
			else if (IsEqualGUID(&wave_format.SubFormat, &KSDATAFORMAT_SUBTYPE_MULAW))
                LOG ("Audio format KSDATAFORMAT_SUBTYPE_MULAW unsupported");
			else if (IsEqualGUID(&wave_format.SubFormat, &KSDATAFORMAT_SUBTYPE_ADPCM))
                LOG ("Audio format KSDATAFORMAT_SUBTYPE_ADPCM unsupported");
		}
	}

	render->lpVtbl->Release(render);
	client->lpVtbl->Release(client);

	return NULL;
}

#define WASAPI_SOUND_FUNC(__funcname__, __type__, __multiplier__) \
void __funcname__ (WAVEFORMATEXTENSIBLE wave_format, HANDLE event_handle, IAudioRenderClient *render, UINT32 frames_per_period) { \
    RefillSampleBuffer (); \
	while (!sound_extern_data.quit) { \
		/* Wait for next buffer event to be signaled.*/ \
		{ DWORD result = WaitForSingleObject(event_handle, INFINITE); assert (result == WAIT_OBJECT_0); if (result != WAIT_OBJECT_0) { LOG ("WaitForSingleObject failed. Error code [%lu] [%s]", result, result == WAIT_ABANDONED ? "WAIT_ABANDONED" : result == WAIT_TIMEOUT ? "WAIT_TIMEOUT" : result == WAIT_FAILED ? "WAIT_FAILED" : "Unknown"); return; } } \
 \
		__type__ *data; \
		HRESULT result = render->lpVtbl->GetBuffer (render, frames_per_period, (BYTE**)&data); \
		switch (result) { \
			case S_OK: break;/*LOG ("ok"); break;*/ \
			case AUDCLNT_E_BUFFER_ERROR: LOG ("AUDCLNT_E_BUFFER_ERROR"); break; \
			case AUDCLNT_E_BUFFER_TOO_LARGE: LOG ("AUDCLNT_E_BUFFER_TOO_LARGE"); break; \
			case AUDCLNT_E_BUFFER_SIZE_ERROR: LOG ("AUDCLNT_E_BUFFER_SIZE_ERROR"); break; \
			case AUDCLNT_E_OUT_OF_ORDER: LOG ("AUDCLNT_E_OUT_OF_ORDER"); break; \
			case AUDCLNT_E_DEVICE_INVALIDATED: LOG ("AUDCLNT_E_DEVICE_INVALIDATED"); break; \
			case AUDCLNT_E_BUFFER_OPERATION_PENDING: LOG ("AUDCLNT_E_BUFFER_OPERATION_PENDING"); break; \
			case AUDCLNT_E_SERVICE_NOT_RUNNING: LOG ("AUDCLNT_E_SERVICE_NOT_RUNNING"); break; \
			case E_POINTER: LOG ("E_POINTER"); break; \
			default: LOG ("Unknown error"); break; \
		} \
        for (int i = 0; i < frames_per_period; ++i) { \
            __type__ sample = sample_buffer[sample_buffer_swap].samples[i] * __multiplier__; \
            repeat (wave_format.Format.nChannels) *data++ = sample; \
        } \
		DO_OR_CONTINUE (render->lpVtbl->ReleaseBuffer (render, frames_per_period, 0), "Failed to release audio buffer"); \
        RefillSampleBuffer (); \
	} \
    LOG ("Render thread exiting normally"); \
}

// Any number of identical channels, 48KHz, 16-bit signed integer
WASAPI_SOUND_FUNC (Sound_Loop_i16, int16_t, INT16_MAX)
// Any number of identical channels, 48KHz, 16-bit signed integer
WASAPI_SOUND_FUNC (Sound_Loop_i8, int8_t, INT8_MAX)
// Any number of identical channels, 48KHz, 16-bit signed integer
WASAPI_SOUND_FUNC (Sound_Loop_i32, int32_t, INT32_MAX)
// Any number of identical channels, 48KHz, 16-bit signed integer
WASAPI_SOUND_FUNC (Sound_Loop_Float, float, 1.f)






#elifdef __APPLE__  // __linux__








#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>

static AudioUnit toneUnit;
OSStatus myAudioObjectPropertyListenerProc (AudioObjectID inObjectID, UInt32 inNumberAddresses, const AudioObjectPropertyAddress *inAddresses,void* __nullable inClientData);

OSStatus SoundCallback (void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData) {
    assert (*ioActionFlags == 0);
    SInt16 *left = (SInt16*)ioData->mBuffers[0].mData;

    // assert (inNumberFrames == sample_buffer_size);
    // LOG ("Audio frames requested: %u", inNumberFrames);

    if (inNumberFrames != sample_buffer_size) {
        LOG ("Reset audio buffer size!");
        UInt32 period_size_in = sample_buffer_size;
        OSErr err = AudioUnitSetProperty(toneUnit, kAudioDevicePropertyBufferFrameSize,
                kAudioUnitScope_Input, 0, &period_size_in, sizeof(period_size_in));
        if (err) LOG("Error setting stream format: %d", err);
    }

    for (UInt32 frame = 0; frame < inNumberFrames; ++frame) {
        // *(left++) = CreateNextSample () * INT16_MAX;
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

#define DO_OR_QUIT(__function__, __error_message__) { OSErr err = __function__; assert (err == 0); if (err != 0) { LOG (__error_message__ "[%d]", err); return NULL; } }

void *Sound (void* args) {
    sample_buffer_size = PERIOD_SIZE;

    RefillSampleBuffer();

    const AudioComponentDescription acd = {
        .componentType = kAudioUnitType_Output,
        .componentSubType = kAudioUnitSubType_DefaultOutput,
        .componentManufacturer = kAudioUnitManufacturer_Apple,
    };
    AudioComponent output;

    { output = AudioComponentFindNext(NULL, &acd); assert (output); if (!output) { LOG("Can't find default output"); return NULL; } }
    DO_OR_QUIT (AudioComponentInstanceNew(output, &toneUnit), "Error creating audio unit");
    AURenderCallbackStruct input = { .inputProc = SoundCallback };
    DO_OR_QUIT (AudioUnitSetProperty(toneUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &input, sizeof(input)), "Error setting render callback");

    AudioStreamBasicDescription asbd = {
        .mFormatID = kAudioFormatLinearPCM,
        .mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked | kAudioFormatFlagIsNonInterleaved,
        .mSampleRate = SAMPLING_RATE,
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