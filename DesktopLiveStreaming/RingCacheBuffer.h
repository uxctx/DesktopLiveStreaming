#pragma once

#include "AppContext.h"
#include "FLVhelp.h"

#define DefaultItemSize (4*1024)
#define rcb_lock_value (4)

enum TagType {
	VideoTag,
	AudioTag,
	ScriptTag,
	OtherTag
};

typedef struct _RCBItem {

	TagType tagType;
	bool isKeyFrame;

	char tag_body_default[DefaultItemSize];//4kb默认
	char *tag_body_big;//
	bool tag_body_isbig;
	size_t tag_body_length;

	char flv_tag_header[20];// size 11+9固定大小 11是tag头 9是视频头 音频也用这个字段

	char *flv_tag_real;//主要是h264的偏移量，真正的数据地址，比如跳过了h264nal的 00 00 00 01
	size_t flv_tag_real_length;//跳过之后的数据长度

	QWORD compositionTime;
	QWORD  timems;
	_RCBItem* next;
	volatile int index;
}RCBItem;

typedef struct FlvTagHeader
{
	char data[12];// size 11固定大小
	WSABUF wsabuf;
};


class RingCacheBuffer
{

public:
	//
	RingCacheBuffer(int ringLength);
	~RingCacheBuffer();

	void overlay_video(bool isKeyFrame, char *data, int length, DWORD timems, int compositionTime);
	void overlay_audio(char *data, int length, DWORD timems);

	friend class FlvLiveStream;
	friend class HLS_Server;

private:
	HANDLE new_event;

	int RingLength;
	volatile RCBItem* LastWrite;//用于实际选择写入

	volatile RCBItem* ReadLastWrite;//用于读取上次写入

	RCBItem* items;
	RCBItem* lastKeyFrame;//上个关键帧

	CRITICAL_SECTION overlay_cs;
	app_atomic_lock_t lock_t;

	volatile unsigned int tag_index;
};