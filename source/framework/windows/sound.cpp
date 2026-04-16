extern "C" {
    #include "sound.h"
    #include "sound_common.h"
}

#include <windows.h>
#include <xaudio2.h>

typedef HRESULT (WINAPI *XAudio2Create_t)(IXAudio2**, UINT32, XAUDIO2_PROCESSOR);

#define PERIOD_SIZE (SAMPLING_RATE / 200)

static IXAudio2* xaudio = nullptr;
static IXAudio2MasteringVoice* xaudio_master_voice = nullptr;
static IXAudio2SourceVoice* xaudio_source_voice = nullptr;
static XAUDIO2_BUFFER xaudio_buffers[2];
static f32 audio_buffers[2][PERIOD_SIZE] = {};

static bool restart_audio = true;

const char *HResultToStr (HRESULT result) {
    static char error_buffer[512];
    DWORD len;  // Number of chars returned.
    len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, result, 0, error_buffer, 512, NULL);
    if (len == 0) {
        if (len == 0) snprintf (error_buffer, sizeof (error_buffer), "HRESULT error message not found");
    }
    return error_buffer;
}

#define DO_OR_QUIT(function_call__, error_message__) do { auto result = function_call__; assert (result == S_OK); if (result != S_OK) { LOG (error_message__ " [%lx] [%s]", result, HResultToStr(result)); return NULL; } } while (0)
#define DO_OR_RESTART(function_call__, error_message__) { auto result = function_call__; if (result != S_OK) { LOG (error_message__ " [%lx] [%s]", result, HResultToStr(result)); restart_audio = true; continue; } }

class VoiceCallback : public IXAudio2VoiceCallback {
public:
    void OnBufferEnd(void* pBufferContext) override {
        memcpy (&audio_buffers[sample_buffer_swap], &sample_buffer[sample_buffer_swap].samples, PERIOD_SIZE * sizeof (f32));
        xaudio_source_voice->SubmitSourceBuffer (&xaudio_buffers[sample_buffer_swap], nullptr);
        RefillSampleBuffer ();
    }

    void OnStreamEnd() {}
    void OnVoiceProcessingPassEnd() {}
    void OnVoiceProcessingPassStart(UINT32 SamplesRequired) {}
    void OnBufferStart(void* pBufferContext) {}
    void OnLoopEnd(void* pBufferContext) {}
    void OnVoiceError(void* pBufferContext, HRESULT Error) {
        restart_audio = true;
        LOG ("Critical XAudio2 voice error [%lx] [%s]", Error, HResultToStr(Error));
    }
};

class EngineCallback : public IXAudio2EngineCallback {
public:
    void OnCriticalError(HRESULT Error) override {
        restart_audio = true;
        LOG ("Critical XAudio2 Engine error [%lx] [%s]", Error, HResultToStr(Error));
    }
    void OnProcessingPassEnd() {}
    void OnProcessingPassStart() {}
};

VoiceCallback xaudio_voice_callback;
EngineCallback xaudio_engine_callback;

const WAVEFORMATEX wave_format = {
    .wFormatTag = WAVE_FORMAT_IEEE_FLOAT,
    .nChannels = 1,
    .nSamplesPerSec = SAMPLING_RATE,
    .nAvgBytesPerSec = SAMPLING_RATE * sizeof (f32),
    .nBlockAlign = sizeof (f32),
    .wBitsPerSample = sizeof (f32) * 8,
    .cbSize = 0
};

void *Sound (void *data_void) {
    sample_buffer_size = PERIOD_SIZE;
    DO_OR_QUIT (CoInitializeEx(NULL, COINIT_MULTITHREADED), "Sound thread failed to CoInitializeEx");

    HMODULE xaudio_dll = nullptr;
    bool multiple_restarts = false;
    while (restart_audio && !sound_extern_data.quit) {
        LOG ("Initializing xaudio");
        if (multiple_restarts) Sleep (1000);
        multiple_restarts = true;

        if (xaudio_source_voice != nullptr) { xaudio_source_voice->DestroyVoice(); xaudio_source_voice = nullptr; }
        if (xaudio_master_voice != nullptr) { xaudio_master_voice->DestroyVoice(); xaudio_master_voice = nullptr; }
        if (xaudio != nullptr) { xaudio->Release(); xaudio = nullptr; }
        if (xaudio_dll) { FreeLibrary (xaudio_dll); xaudio_dll = nullptr; }

        xaudio_dll = LoadLibraryA("xaudio2_9.dll");
        if (!xaudio_dll) { LOG("Failed to load xaudio2_9.dll"); restart_audio = true; continue; }

        XAudio2Create_t my_XAudio2Create = (XAudio2Create_t)GetProcAddress (xaudio_dll, "XAudio2Create"); assert (my_XAudio2Create);
        if (!my_XAudio2Create) { LOG ("Failed to get XAudio2Create"); restart_audio = true; continue; }

        DO_OR_RESTART (my_XAudio2Create(&xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR), "Sound thread failed to create XAudio2");

        DO_OR_RESTART (xaudio->RegisterForCallbacks(&xaudio_engine_callback), "Failed to register for engine callbacks");

        DO_OR_RESTART (xaudio->CreateMasteringVoice(&xaudio_master_voice, 1, SAMPLING_RATE, 0, NULL, NULL, AudioCategory_GameEffects), "Failed to create mastering voice");

        DO_OR_RESTART (xaudio->CreateSourceVoice(&xaudio_source_voice, &wave_format, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &xaudio_voice_callback, NULL, NULL), "Failed to create source voice");

        multiple_restarts = restart_audio = false;

        xaudio_source_voice->Start(0, XAUDIO2_COMMIT_NOW);

        memset (audio_buffers, 0, sizeof (audio_buffers));
        xaudio_buffers[0].AudioBytes = xaudio_buffers[1].AudioBytes = sizeof(f32) * PERIOD_SIZE;
        xaudio_buffers[0].pAudioData = (const BYTE*)&audio_buffers[0];
        xaudio_buffers[1].pAudioData = (const BYTE*)&audio_buffers[1];

        xaudio_source_voice->SubmitSourceBuffer (&xaudio_buffers[0], nullptr);
        xaudio_source_voice->SubmitSourceBuffer (&xaudio_buffers[1], nullptr);
        
        LOG ("XAudio starting keepalive loop");
        while (!sound_extern_data.quit && !restart_audio) Sleep (500);
        LOG ("Xaudio keepalive %s", sound_extern_data.quit ? "quitting" : (restart_audio ? "restarting" : "!unknown!"));
    }

    CoUninitialize();

    LOG ("Sound thread exiting");

	return NULL;
}