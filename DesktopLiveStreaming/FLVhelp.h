#pragma once

#include "amf.h"
#include "AppContext.h"

#define SAVC(x)    static const AVal av_##x = AVC(#x)

SAVC(onMetaData);
SAVC(duration);
SAVC(width);
SAVC(height);
SAVC(videocodecid);
SAVC(videodatarate);
SAVC(framerate);
SAVC(audiocodecid);
SAVC(audiodatarate);
SAVC(audiosamplerate);
SAVC(audiosamplesize);
SAVC(audiochannels);
SAVC(stereo);
SAVC(encoder);
SAVC(filesize);

SAVC(avc1);
SAVC(mp4a);

#define TAGHEADNERSIZE 11

static const AVal av_encoder_str = { AppnName,strlen(AppnName) };// flv中的编码器名称
typedef struct flv_val
{
	char *val;
	int len;
} flv_val;

static unsigned char header_data[] = { 0x46,0x4C,0x56,0x01,0x05,0x00,0x00,0x00,0x09,   0x00,0x00,0x00,0x00 };//flv的header+previoustagsize
static flv_val flv_val_header = { (char*)header_data ,13 };


class FLVhelp
{
public:
	FLVhelp();
	~FLVhelp();

	static char*  FLVPacket(char* stream, char *data, UINT size, BYTE type, DWORD timestamp);

	//<350kb的大小
	static char* HeaderAndMetaDataTag(char* packet,
		double width, double height,
		double audiodatarate, double audiosamplerate, double audiosamplesize, double audiochannels);
	static void FLVPacketHeader(char* packet, UINT size, BYTE type, DWORD timestamp);
};

