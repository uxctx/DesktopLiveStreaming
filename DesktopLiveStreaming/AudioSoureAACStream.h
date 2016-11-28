#pragma once
#define WIN32_LEAN_AND_MEAN
#include "AppContext.h"
#include "LoopbackAudioSource.h"

#include <MMSystem.h>  
#include <mmdeviceapi.h>  
#include <endpointvolume.h>  
#include <Functiondiscoverykeys_devpkey.h>//PKEY_Device_FriendlyName  

#include <Audioclient.h>
extern "C"
{
#include "libfaac/include/faac.h"
#include "libfaac/include/faaccfg.h"
}

class AudioSoureAACStream
{

private:
	LoopbackAudioSource *lbas_output;
	PWAVEFORMATEX outfmt;

	LoopbackAudioSource *lbas_input;
	PWAVEFORMATEX infmt;

	faacEncHandle faac;
	DWORD numReadSamples;
	DWORD outputSize;

	
	HANDLE threadHandle[1];
public:
	AudioSoureAACStream(UINT bitRate);
	~AudioSoureAACStream();

	void EnableMicrophone(bool enable = true);

	bool Strat();
	void Stop();
	void ThreadFun();
	int GetHeander(char* data);
	PWAVEFORMATEX GetFmt();

	bool mcrophoneInput = true;
	bool can_work;
private:
	bool theadrun = false;
	

	char header[8];
	int headerLen;
	char *accInbuffer;
	unsigned char *out_buffer;
	void EncodeFun();
	
};

