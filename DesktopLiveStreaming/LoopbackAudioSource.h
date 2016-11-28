#pragma once
#define WIN32_LEAN_AND_MEAN
#include <atlbase.h>
#include <Audioclient.h>
#include <mmdeviceapi.h>

#include <memory>
#include <utility>

enum AudioSourceEnum
{
	INPUTAUDIO,
	OUTPUTAUDIO,
};

class LoopbackAudioSource {

public:
	LoopbackAudioSource(AudioSourceEnum type);
	~LoopbackAudioSource();
	PWAVEFORMATEX format() { return format_; }
	void Read(BYTE **data, UINT32*num_frames);
	void ReleaseBuffer();

	friend class AudioSoureAACStream;
private:
	CComPtr<IAudioClient> event_client_;
	CComPtr<IAudioClient> loopback_client_;
	CComPtr<IAudioCaptureClient> capture_client_;
	HANDLE event_handle_;
	PWAVEFORMATEX format_;
	int pNumFramesToRead;

	bool CanWork;

	
};
