#include "stdafx.h"
#include "AudioSoureAACStream.h"
#include "RingCacheBuffer.h"

UINT _stdcall AudioSoureEncodeThread(LPVOID lParam)
{
	AudioSoureAACStream *as = static_cast<AudioSoureAACStream*>(lParam);
	as->ThreadFun();
	return 0;
};

AudioSoureAACStream::AudioSoureAACStream(UINT bitRate)
{

	lbas_output = new LoopbackAudioSource(AudioSourceEnum::OUTPUTAUDIO);
	lbas_input = new LoopbackAudioSource(AudioSourceEnum::INPUTAUDIO);

	if (lbas_output->CanWork)
		outfmt = lbas_output->format();
	if (lbas_input->CanWork)
		infmt = lbas_input->format();

	if (!lbas_output->CanWork && !lbas_input->CanWork)
	{
		can_work = false;
		return;
	}
	else
	{
		can_work = true;
	}
	faac = faacEncOpen(outfmt->nSamplesPerSec
		, outfmt->nChannels, &numReadSamples, &outputSize);

	faacEncConfigurationPtr config = faacEncGetCurrentConfiguration(faac);
	config->bitRate = (bitRate * 1000) / 2;//64000
	config->quantqual = 100;
	config->inputFormat = FAAC_INPUT_FLOAT;
	config->mpegVersion = MPEG4;
	config->aacObjectType = LOW;
	config->useLfe = 0;
	//(0 = Raw ; 1 = ADTS 单独aac播放) 
	config->outputFormat = 0;

	BYTE *tempHeader;
	DWORD len;
	header[0] = 0xaf;
	header[1] = 0x00;
	int ret = faacEncSetConfiguration(faac, config);
	ret = faacEncGetDecoderSpecificInfo(faac, &tempHeader, &len);

	memcpy(((char*)header) + 2, tempHeader, len);
	this->headerLen = 2 + len;
	free(tempHeader);


	if ((lbas_output->CanWork && lbas_input->CanWork) && (outfmt->nChannels == infmt->nChannels&&
		outfmt->nSamplesPerSec == infmt->nSamplesPerSec&&
		outfmt->nAvgBytesPerSec == infmt->nAvgBytesPerSec&&
		outfmt->wBitsPerSample == infmt->wBitsPerSample&&
		outfmt->nBlockAlign == infmt->nBlockAlign)
		)
		this->mcrophoneInput = true;
	else
		this->mcrophoneInput = false;

	accInbuffer = (char *)_aligned_malloc(numReadSamples * 4, 16);
	out_buffer = (unsigned char *)app_malloc(outputSize + 2);

	out_buffer[0] = 0xaf;
	out_buffer[1] = 0x1;
}

AudioSoureAACStream::~AudioSoureAACStream()
{
	_aligned_free(accInbuffer);
	app_free(out_buffer);
	faacEncClose(faac);
	delete lbas_output;
}

void AudioSoureAACStream::EnableMicrophone(bool enable)
{
}

PWAVEFORMATEX AudioSoureAACStream::GetFmt()
{
	if (lbas_output->CanWork)
		return outfmt;
	if (lbas_input->CanWork)
		return	infmt;

}

bool AudioSoureAACStream::Strat()
{
	if (!can_work)
		return false;
	theadrun = true;
	threadHandle[0] = (HANDLE)_beginthreadex(NULL, 0, AudioSoureEncodeThread, this, 0, NULL);
	return false;
}

void AudioSoureAACStream::Stop()
{
	theadrun = false;
	WaitForMultipleObjects(1, threadHandle, TRUE, INFINITE);
	CloseHandle(threadHandle[0]);
}

void AudioSoureAACStream::ThreadFun()
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
	int dataBlockSize = outfmt->nChannels* (outfmt->wBitsPerSample / 8);
	int insize = numReadSamples * 4;//acc编码每次输入大小

	PBYTE data;
	UINT32 num_frames;
	int datasize = 0;
	char tempFrameOut[1024 * 8];
	char tempFrameIn[1024 * 8];

	int nowindex = 0, copycount = 0;

	while (theadrun)
	{
		lbas_output->Read(&data, &num_frames);
		datasize = num_frames*dataBlockSize;
		memcpy(tempFrameOut, data, datasize);
		lbas_output->ReleaseBuffer();
		if (this->mcrophoneInput)
		{
			lbas_input->Read(&data, &num_frames);

			if (num_frames*dataBlockSize <= datasize)
			{
				memcpy(tempFrameIn, data, datasize);
				lbas_input->ReleaseBuffer();
				float* out = (float*)tempFrameOut;
				float* in = (float*)tempFrameIn;
				for (size_t i = 0; i < num_frames*outfmt->nChannels; i++){
					if (*in != 0.0){
						if (*out == 0.0)
							*out = *in;
						else
							*out = (*out + *in) / 2;
					}

					out++;
					in++;
				}
			}
			lbas_input->ReleaseBuffer();
		}

		copycount = insize - (nowindex + datasize);
		if (copycount > 0)
		{
			memcpy(accInbuffer + nowindex, tempFrameOut, datasize);
			nowindex += datasize;
			continue;
		}
		else if (copycount < 0)
		{
			int prosize = insize - nowindex;
			memcpy(accInbuffer + nowindex, tempFrameOut, prosize);
			this->EncodeFun();
			memcpy(accInbuffer, tempFrameOut + prosize, -copycount);
			nowindex = -copycount;
		}
		else
		{
			memcpy(accInbuffer + nowindex, tempFrameOut, insize - nowindex);
			this->EncodeFun();
			nowindex = 0;
		}

	}
}

int AudioSoureAACStream::GetHeander(char * data)
{
	memcpy(data, this->header, headerLen);
	return this->headerLen;
}

void AudioSoureAACStream::EncodeFun()
{
	RingCacheBuffer* cache = MyContext->rCache;
	UINT floatsLeft = numReadSamples;

	float *inputTemp = (float*)accInbuffer;
	//方式1
	if (((unsigned long)(inputTemp) & 0xF) == 0)//指针地制是否为16字节对齐
	{
		UINT alignedFloats = floatsLeft & 0xFFFFFFFC;
		for (UINT i = 0; i < alignedFloats; i += 4)
		{
			float *pos = inputTemp + i;
			_mm_store_ps(pos, _mm_mul_ps(_mm_load_ps(pos), _mm_set_ps1(32767.0f)));
		}

		floatsLeft &= 0x3;
		inputTemp += alignedFloats;
	}

	//方式2
	if (floatsLeft)
	{
		for (UINT i = 0; i < floatsLeft; i++)
			inputTemp[i] *= 32767.0f;
	}

	int ret = faacEncEncode(faac, (int32_t*)accInbuffer, numReadSamples, out_buffer + 2, outputSize);

	QWORD nowMs = GetQPCTimeMS();
	cache->overlay_audio((char*)out_buffer, ret + 2, nowMs);

}


