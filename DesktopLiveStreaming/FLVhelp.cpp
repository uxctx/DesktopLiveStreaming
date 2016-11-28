#include "stdafx.h"
#include "FLVhelp.h"
#include "AppContext.h"

FLVhelp::FLVhelp()
{
}

FLVhelp::~FLVhelp()
{
}


char* FLVhelp::HeaderAndMetaDataTag(char* packet,
	double width, double height,
	double audiodatarate, double audiosamplerate, double audiosamplesize, double audiochannels)
{

	memcpy(packet, flv_val_header.val, flv_val_header.len);
	packet += flv_val_header.len;

	//metadata

	char  metaDataBuffer[2048];
	char *enc = metaDataBuffer;
	char *pend = metaDataBuffer + sizeof(metaDataBuffer);

	enc = AMF_EncodeString(enc, pend, &av_onMetaData);

	*enc++ = AMF_ECMA_ARRAY;
	enc = AMF_EncodeInt32(enc, pend, 13);
	//*enc++ = AMF_OBJECT;

	enc = AMF_EncodeNamedNumber(enc, pend, &av_duration, 0.0);
	enc = AMF_EncodeNamedNumber(enc, pend, &av_filesize, 0.0);
	enc = AMF_EncodeNamedNumber(enc, pend, &av_width, width);
	enc = AMF_EncodeNamedNumber(enc, pend, &av_height, height);

	//enc = AMF_EncodeNamedString(enc, pend, &av_videocodecid, &av_avc1);//7.0);//
	enc = AMF_EncodeNamedNumber(enc, pend, &av_videocodecid, 7.0);//7.0 mp4a ->h*264

	enc = AMF_EncodeNamedNumber(enc, pend, &av_videodatarate, 4000);

	//enc = AMF_EncodeNamedString(enc, pend, &av_audiocodecid, &av_mp4a);//10.0 aac
	enc = AMF_EncodeNamedNumber(enc, pend, &av_audiocodecid, 10.0);//10.0 aac

	enc = AMF_EncodeNamedNumber(enc, pend, &av_audiodatarate, audiodatarate);
	enc = AMF_EncodeNamedNumber(enc, pend, &av_audiosamplerate, audiosamplerate);
	enc = AMF_EncodeNamedNumber(enc, pend, &av_audiosamplesize, audiosamplesize);
	enc = AMF_EncodeNamedNumber(enc, pend, &av_audiochannels, audiochannels);

	enc = AMF_EncodeNamedBoolean(enc, pend, &av_stereo, audiochannels == 2);

	enc = AMF_EncodeNamedString(enc, pend, &av_encoder, &av_encoder_str);
	//end
	*enc++ = 0;
	*enc++ = 0;
	*enc++ = AMF_OBJECT_END;

	return FLVhelp::FLVPacket(packet, metaDataBuffer, enc - metaDataBuffer, 18, 0);

}

char* FLVhelp::FLVPacket(char* packet, char *data, UINT size, BYTE type, DWORD timestamp)
{

	UINT networkTimestamp = fastHtonl(timestamp);

	UINT streamID = 0;

	*packet++ = type;

	packet = EncodeInt24(packet, size);//3

	//time tamp-> [time tamp 3b,time tamp ex 1b]
	memcpy(packet, ((LPBYTE)(&networkTimestamp)) + 1, 3);
	packet += 3;

	memcpy(packet, &networkTimestamp, 1);
	packet += 1;

	packet += 3;// 1 -> 3, ¡ý

	memcpy(packet, &streamID, 3);
	//packet += 3;

	memcpy(packet, data, size);
	packet += size;

	//memcpy(packet, &allsize,4);
	return	EncodeInt32(packet, size + 11);

	//11  =  TYPE 1  + Size 3 + timestamp 4 +streamID 3 

}

void FLVhelp::FLVPacketHeader(char* packet, UINT size, BYTE type, DWORD timestamp)
{

	UINT networkTimestamp = fastHtonl(timestamp);

	UINT streamID = 0;

	*packet++ = type;

	packet = EncodeInt24(packet, size);//3
									   
	memcpy(packet, ((LPBYTE)(&networkTimestamp)) + 1, 3);
	//time tamp-> [time tamp 3b,time tamp ex 1b]
	packet += 3;

	memcpy(packet, &networkTimestamp, 1);
	packet += 1;

	packet += 3;// 1 -> 3, ¡ý

	memcpy(packet, &streamID, 3);

}