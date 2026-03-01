#ifndef FUNCNAME__
#error "Must #define FUNCNAME__ before #including this file"
#endif
#ifndef TYPE__
#error "Must #define TYPE__ before #including this file"
#endif

void FUNCNAME__ (WAVEFORMATEXTENSIBLE wave_format, HANDLE event_handle, IAudioRenderClient *render, UINT32 frames_per_period) {
    TYPE__ maxval;
    _Generic(TYPE__, 
        float: maxval = 1,
        int32_t: maxval = (1 << (wave_format.Samples.wValidBitsPerSample-1))
    );
    uint8_t bytes_per_sample = wave_format.Format.wBitsPerSample / 8;

    RefillSampleBuffer ();

	while (!sound_extern_data.quit && !restart_audio) {
		{ // Wait for next buffer event to be signaled.
            DWORD result = WaitForSingleObject(event_handle, 1000);
            if (result != WAIT_OBJECT_0) {
                LOG ("WaitForSingleObject failed. Error code [%lu] [%s]", result, result == WAIT_ABANDONED ? "WAIT_ABANDONED" : result == WAIT_TIMEOUT ? "WAIT_TIMEOUT" : result == WAIT_FAILED ? "WAIT_FAILED" : "Unknown");
                restart_audio = true;
                break;
            }
        }

		BYTE *data;
		HRESULT result = render->lpVtbl->GetBuffer (render, frames_per_period, (BYTE**)&data);
		switch (result) {
			case S_OK: break;
			case AUDCLNT_E_BUFFER_ERROR: LOG ("AUDCLNT_E_BUFFER_ERROR"); break;
			case AUDCLNT_E_BUFFER_TOO_LARGE: LOG ("AUDCLNT_E_BUFFER_TOO_LARGE"); break;
			case AUDCLNT_E_BUFFER_SIZE_ERROR: LOG ("AUDCLNT_E_BUFFER_SIZE_ERROR"); break;
			case AUDCLNT_E_OUT_OF_ORDER: LOG ("AUDCLNT_E_OUT_OF_ORDER"); break;
			case AUDCLNT_E_DEVICE_INVALIDATED: LOG ("AUDCLNT_E_DEVICE_INVALIDATED"); restart_audio = true; break;
			case AUDCLNT_E_BUFFER_OPERATION_PENDING: LOG ("AUDCLNT_E_BUFFER_OPERATION_PENDING"); break;
			case AUDCLNT_E_SERVICE_NOT_RUNNING: LOG ("AUDCLNT_E_SERVICE_NOT_RUNNING"); restart_audio = true; break;
			case E_POINTER: LOG ("E_POINTER"); restart_audio = true; break;
			default: LOG ("Unknown error"); restart_audio = true; break;
		}
        if (result != S_OK) continue;

        for (int i = 0; i < frames_per_period; ++i) {
            TYPE__ sample = sample_buffer[sample_buffer_swap].samples[i] * maxval;
            repeat (wave_format.Format.nChannels) {
                *(TYPE__*)data = sample;
                data += bytes_per_sample;
            }
        }
		DO_OR_CONTINUE (render->lpVtbl->ReleaseBuffer (render, frames_per_period, 0), "Failed to release audio buffer");

        RefillSampleBuffer ();
	}
    LOG ("WASAPI sound func exiting");
}
