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
#include "DEFER.h"

#import <AudioToolbox/AudioToolbox.h>
#import <AudioUnit/AudioUnit.h>

// 5ms buffer
#define PERIOD_SIZE (SAMPLING_RATE / 200)

#define DO_OR_RETRY(__function__, __error_message__) { OSErr err = __function__; assert (err == 0); if (err != 0) { LOG (__error_message__ "[%d]", err); continue; } }
#define DO_OR_LOG(__function__, __error_message__) { OSErr err = __function__; if (err != 0) { LOG (__error_message__ "[%d]", err); } }

static bool restart_audio = true;

OSStatus SoundCallback (void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData) {
    SInt16 *left = (SInt16*)ioData->mBuffers[0].mData;

    for (UInt32 frame = 0; frame < inNumberFrames; ++frame) {
        *(left++) = sample_buffer[sample_buffer_swap].samples[frame] * INT16_MAX;
    }

    RefillSampleBuffer();

    return noErr;
}

OSStatus deviceChangedCallback(AudioObjectID inObjectID, UInt32 inNumberAddresses, const AudioObjectPropertyAddress inAddresses[], void* inClientData) {
    LOG ("Default output device changed.\n");
    restart_audio = true;
    return noErr;
}

void *Sound (void* args) {
    sample_buffer_size = PERIOD_SIZE;

    RefillSampleBuffer();

    bool restart_multiple_attempts = false;
    while (restart_audio && !sound_extern_data.quit) {
        if (restart_multiple_attempts > 0) sleep (1);
        restart_multiple_attempts = true;

        AudioComponent output;
        { output = AudioComponentFindNext(NULL,
            &(AudioComponentDescription){
                .componentType = kAudioUnitType_Output,
                .componentSubType = kAudioUnitSubType_DefaultOutput,
        }); assert (output); if (!output) { LOG("Can't find default output"); continue; } }

        AudioUnit tone_unit;
        DO_OR_RETRY (AudioComponentInstanceNew(output, &tone_unit), "Error creating audio unit");
        defer { DO_OR_LOG (AudioComponentInstanceDispose(tone_unit), "Error disposing audio unit"); }

        DO_OR_RETRY (AudioUnitSetProperty(tone_unit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &(AURenderCallbackStruct){.inputProc = SoundCallback}, sizeof(AURenderCallbackStruct)), "Error setting render callback");

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
        DO_OR_RETRY (AudioUnitSetProperty(tone_unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &asbd, sizeof(asbd)), "Error setting stream format");

        DO_OR_LOG (AudioUnitSetProperty(tone_unit, kAudioDevicePropertyBufferFrameSize, kAudioUnitScope_Input, 0, &(UInt32){PERIOD_SIZE}, sizeof(UInt32)), "Error setting buffer size");

        AudioObjectPropertyAddress property_address = {
            .mSelector = kAudioHardwarePropertyDefaultOutputDevice,
            .mScope = kAudioObjectPropertyScopeGlobal,
            .mElement = kAudioObjectPropertyElementMain
        };
        DO_OR_LOG (AudioObjectAddPropertyListener (kAudioObjectSystemObject, &property_address, deviceChangedCallback, NULL), "Failed to add property listener for default device change");
        defer { DO_OR_LOG (AudioObjectRemovePropertyListener (kAudioObjectSystemObject, &property_address, deviceChangedCallback, NULL), "Failed to remove property listener for default device change"); }

        DO_OR_RETRY (AudioUnitInitialize(tone_unit), "Error initializing unit");
        defer { DO_OR_LOG (AudioUnitUninitialize(tone_unit), "Error uninitializing unit"); }

        DO_OR_RETRY (AudioOutputUnitStart(tone_unit), "Error starting unit");
        defer { DO_OR_LOG (AudioOutputUnitStop(tone_unit), "Error stopping unit"); }

        restart_audio = restart_multiple_attempts = false;
        
        while (!sound_extern_data.quit && !restart_audio) {
            sleep (1);

            UInt32 is_running = 0;
            if (AudioUnitGetProperty(tone_unit, kAudioOutputUnitProperty_IsRunning, kAudioUnitScope_Global, 0, &is_running, &(UInt32){sizeof(is_running)}) != noErr || !is_running) {
                LOG ("Device not running");
                restart_audio = true;
                break;
            }
        }
    }

    return NULL;
}
