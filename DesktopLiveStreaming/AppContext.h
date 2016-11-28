#pragma once
#ifndef _APPCONTEXT
#define _APPCONTEXT

#include "CommonFun.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winsock2.h> 
#include <mswsock.h>
#include <Ws2tcpip.h>
#include <process.h> 
#include <stdint.h>

extern "C"
{
#include "x264/x264.h"
#include "x264/x264_config.h"

#include "atomic_lock.h"
	
};

#include "amf.h"

typedef unsigned long long  QWORD, ULONG64, UINT64, ULONGLONG;

#define Development 

#define FLVLIVEURL "/live.flv"
#define HLSM3U8URL "/live.m3u8"

#define GETMODEL "/getmode"

static char* AppnName = "desktopwebshare 1.0";
static LARGE_INTEGER clockFreq;

#define FPS (30)
#define AACbitRate  (128)

#define heap_malloc(s) HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(s))
#define heap_free(p)   HeapFree(GetProcessHeap(),0,(p))

#define QWORD_BE(val) (((val>>56)&0xFF) | (((val>>48)&0xFF)<<8) | (((val>>40)&0xFF)<<16) | (((val>>32)&0xFF)<<24) | \
    (((val>>24)&0xFF)<<32) | (((val>>16)&0xFF)<<40) | (((val>>8)&0xFF)<<48) | ((val&0xFF)<<56))
#define DWORD_BE(val) (((val>>24)&0xFF) | (((val>>16)&0xFF)<<8) | (((val>>8)&0xFF)<<16) | ((val&0xFF)<<24))
#define WORD_BE(val)  (((val>>8)&0xFF) | ((val&0xFF)<<8))

__forceinline DWORD fastHtonl(DWORD dw) { return DWORD_BE(dw); }
__forceinline  WORD fastHtons(WORD  w) { return  WORD_BE(w); }

__forceinline void*
app_memcpy(void * dst, void*src, size_t size)
{
	memcpy(dst, src, size);
	return (char*)dst + size;
}

inline char * EncodeInt24(char *output, int nVal)
{

	output[2] = nVal & 0xff;
	output[1] = nVal >> 8;
	output[0] = nVal >> 16;
	return output + 3;
}

inline char * EncodeInt32(char *output, int nVal)
{
	output[3] = nVal & 0xff;
	output[2] = nVal >> 8;
	output[1] = nVal >> 16;
	output[0] = nVal >> 24;
	return output + 4;
}



 struct data_info {
	char* data;
	size_t len;
};

 struct static_text {
	const char* str;
	size_t len;
};
#define STC(str)	{str,strlen(str)}

void CrashErrorWIN32(const char *text);

void CrashErrorMessage(const char *text);

void __cdecl DbgPrintA(const char *format, ...);

void AppWSAStartup();

void* app_malloc(size_t size);

void app_free(void* data);

QWORD GetQPCTimeNS();
QWORD GetQPCTimeMS();

bool  SleepToNS(QWORD qwNSTime);

class DesktopImageSource;
class AudioSoureAACStream;
class H264_Encoder;
class RingCacheBuffer;
class FlvLiveStream;
class HLS_Server;

enum LiveModel
{
	http_flv,
	hls,
};

class AppContext
{
	friend class DesktopImageSource;
private:

	int video_width;
	int video_height;

public:

	static AppContext *context;//不想用单例

	AppContext();
	~AppContext();

	void Log(int level, wchar_t* logtext);
	void SetVideoSize(int width, int height);
	int GetWidth();
	int GetHeight();

	//(ˋДˊ)

	H264_Encoder *h264Encoder;
	DesktopImageSource *desktopSource;
	AudioSoureAACStream* accSource;
	RingCacheBuffer* rCache;
	FlvLiveStream *flvServer;
	HLS_Server* hlsServer;

	LiveModel live_model;
};

#define MyContext (AppContext::context)


#endif // !1