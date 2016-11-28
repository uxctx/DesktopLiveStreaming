#include <cassert>

#include "LoopbackAudioSource.h"
#include "AppContext.h"
LoopbackAudioSource::LoopbackAudioSource(AudioSourceEnum type)
{
	this->CanWork = true;
	HRESULT hr;
	CComPtr<IMMDeviceEnumerator> enumerator;
	hr = enumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator));
	assert(SUCCEEDED(hr));
	CComPtr<IMMDevice> device;
	if (type == AudioSourceEnum::INPUTAUDIO)
	{
		hr = enumerator->GetDefaultAudioEndpoint(eCapture, eCommunications, &device);
	}
	else
	{
		hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
	}
	if (!SUCCEEDED(hr))
	{
		this->CanWork = false;
		return;
	}
	assert(SUCCEEDED(hr));

	hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void **)&event_client_);
	assert(SUCCEEDED(hr));
	hr = event_client_->GetMixFormat(&format_);
	assert(SUCCEEDED(hr));
	hr = event_client_->Initialize(AUDCLNT_SHAREMODE_SHARED,
		AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
		0, 0, format_, nullptr);
	assert(SUCCEEDED(hr));
	event_handle_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
	assert(event_handle_);
	hr = event_client_->SetEventHandle(event_handle_);
	assert(SUCCEEDED(hr));
	hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void **)&loopback_client_);
	assert(SUCCEEDED(hr));
	hr = loopback_client_->Initialize(AUDCLNT_SHAREMODE_SHARED,
		type == AudioSourceEnum::INPUTAUDIO ? 0 : AUDCLNT_STREAMFLAGS_LOOPBACK,
		0, 0, format_, nullptr);
	assert(SUCCEEDED(hr));
	hr = loopback_client_->GetService(__uuidof(IAudioCaptureClient), (void **)&capture_client_);
	assert(SUCCEEDED(hr));
	hr = event_client_->Start();
	assert(SUCCEEDED(hr));
	hr = loopback_client_->Start();
	assert(SUCCEEDED(hr));
}

LoopbackAudioSource::~LoopbackAudioSource()
{
	BOOL ret = CloseHandle(event_handle_);
	assert(ret);
}

void LoopbackAudioSource::ReleaseBuffer()
{
	capture_client_->ReleaseBuffer(pNumFramesToRead);
}

void LoopbackAudioSource::Read(BYTE **data, UINT32*num_frames)
{

	DWORD flags;
	HRESULT hr;
	UINT size;

	while (true) {
		hr = capture_client_->GetNextPacketSize(&size);


		assert(SUCCEEDED(hr));
		hr = capture_client_->GetBuffer(data, num_frames, &flags, nullptr, nullptr);
		assert(SUCCEEDED(hr) || hr == AUDCLNT_E_OUT_OF_ORDER);
		if (hr == S_OK) {
			pNumFramesToRead = *num_frames;
			return;
		}
		DWORD ret = WaitForSingleObject(event_handle_, INFINITE);  // TODO(iceboy): Event & event multiplex.
		assert(ret == WAIT_OBJECT_0);
	}
}

