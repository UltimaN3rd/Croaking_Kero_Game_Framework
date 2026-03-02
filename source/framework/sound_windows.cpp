extern "C" {
#include "sound.h"
}
#include <windows.h>
#include <xaudio2.h>

typedef HRESULT (WINAPI *XAudio2Create_t)(IXAudio2**, UINT32, XAUDIO2_PROCESSOR);

#define SAMPLING_RATE 48000
#define PERIOD_SIZE (SAMPLING_RATE / 200)

static IXAudio2* xaudio;
static IXAudio2MasteringVoice* xaudioMasterVoice;
static IXAudio2SourceVoice* xaudioSourceVoice;
static XAUDIO2_BUFFER xaudioBuffers[2];
static float audio_buffers[2][SAMPLING_RATE] = {};
static bool xaudio_buffer_swap = 0;

static bool restart_audio = true;

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

class VoiceCallback : public IXAudio2VoiceCallback {
public:
    void OnBufferEnd(void* pBufferContext) override {
        puts ("OnBufferEnd");
        xaudioSourceVoice->SubmitSourceBuffer (&xaudioBuffers[xaudio_buffer_swap], nullptr);
        xaudio_buffer_swap = !xaudio_buffer_swap;
    }

    void OnStreamEnd() {puts ("OnStreamEnd");}
    void OnVoiceProcessingPassEnd() {puts("OnVoiceProcessingPassEnd");}
    void OnVoiceProcessingPassStart(UINT32 SamplesRequired) {puts("OnVoiceProcessingPassStart");}
    void OnBufferStart(void* pBufferContext) {puts("OnBufferStart");}
    void OnLoopEnd(void* pBufferContext) {puts("OnLoopEnd");}
    void OnVoiceError(void* pBufferContext, HRESULT Error) {puts("OnVoiceError");}
};

VoiceCallback xaudio_voice_callback;
const WAVEFORMATEX wave_format = {
    .wFormatTag = WAVE_FORMAT_IEEE_FLOAT,
    .nChannels = 1,
    .nSamplesPerSec = SAMPLING_RATE,
    .nAvgBytesPerSec = SAMPLING_RATE * sizeof (float),
    .nBlockAlign = sizeof (float),
    .wBitsPerSample = sizeof (float) * 8,
    .cbSize = 0
};

void *Sound (void *data_void) {
    DO_OR_QUIT (CoInitializeEx(NULL, 0), "Sound thread failed to CoInitializeEx");

    HMODULE xaudio_dll = LoadLibraryA("xaudio2_9.dll");
    if (!xaudio_dll) {
        LOG("Failed to load xaudio2_9.dll");
        return NULL;
    }

    XAudio2Create_t my_XAudio2Create = (XAudio2Create_t)GetProcAddress (xaudio_dll, "XAudio2Create"); assert (my_XAudio2Create);
    if (!my_XAudio2Create) {
        LOG ("Failed to get XAudio2Create");
        return NULL;
    }

    DO_OR_QUIT (my_XAudio2Create(&xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR), "Sound thread failed to create XAudio2");

    DO_OR_QUIT ( xaudio->CreateMasteringVoice(&xaudioMasterVoice, 1, SAMPLING_RATE, 0, NULL, NULL, AudioCategory_GameEffects), "Failed to create mastering voice");

    xaudio->CreateSourceVoice(&xaudioSourceVoice, &wave_format, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &xaudio_voice_callback, NULL, NULL);

    xaudioSourceVoice->Start(0, XAUDIO2_COMMIT_NOW);

    uint32_t wave_counter = 0;
    float val = 0;
    for (int i = 0; i < SAMPLING_RATE; ++i) {
        audio_buffers[0][i] = audio_buffers[1][i] = val;
        if (++wave_counter == SAMPLING_RATE / 440) {
            wave_counter = 0;
            val = (val == 0) ? 1 : 0;
        }
    }

    xaudioBuffers[0].AudioBytes = xaudioBuffers[1].AudioBytes = sizeof(float) * PERIOD_SIZE;
    xaudioBuffers[0].pAudioData = (const BYTE*)&audio_buffers[0];
    xaudioBuffers[1].pAudioData = (const BYTE*)&audio_buffers[1];

    xaudioSourceVoice->SubmitSourceBuffer (&xaudioBuffers[xaudio_buffer_swap], nullptr);
    xaudio_buffer_swap = !xaudio_buffer_swap;

    CoUninitialize();

    LOG ("Sound thread exiting");

	return NULL;
}