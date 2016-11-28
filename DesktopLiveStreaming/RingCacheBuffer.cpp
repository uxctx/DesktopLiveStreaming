#include "RingCacheBuffer.h"
#include "AppContext.h"

RingCacheBuffer::RingCacheBuffer(int ringLength)
{
	this->RingLength = ringLength;

	this->new_event = CreateEvent(NULL, FALSE, TRUE, L"RCB_EVENT");
	if (this->new_event == NULL)
	{
		CrashErrorWIN32("CreateEvent");
		return;
	}

	this->items = (RCBItem *)app_malloc(ringLength * sizeof(RCBItem));
	memset(this->items, 0, ringLength * sizeof(RCBItem));

	RCBItem* tempRcb = this->items;
	for (size_t i = 1; i < this->RingLength; i++)
	{
		tempRcb->index = 0;
		tempRcb->tag_body_isbig = false;

		tempRcb->next = tempRcb + 1;
		tempRcb = tempRcb->next;

	}
	tempRcb->index = this->RingLength;
	tempRcb->tag_body_isbig = false;
	tempRcb->next = this->items;
	this->LastWrite = this->items;
	InitializeCriticalSection(&overlay_cs);

	this->lastKeyFrame = NULL;
	lock_t.lock = 0;

}

RingCacheBuffer::~RingCacheBuffer()
{
	app_free(this->items);
	DeleteCriticalSection(&overlay_cs);
}

void RingCacheBuffer::overlay_video(bool isKeyFrame, char * data, int length, DWORD timems, int  compositionTime)
{
	

	//EnterCriticalSection(&overlay_cs);
	app_atomic_lock(&this->lock_t,rcb_lock_value);
	RCBItem* temp_rcb = (RCBItem*)this->LastWrite;
	temp_rcb = temp_rcb->next;
	this->LastWrite = temp_rcb;
	temp_rcb->index = this->tag_index++;
	//LeaveCriticalSection(&overlay_cs);
	app_atomic_unlock(&this->lock_t, rcb_lock_value);

	temp_rcb->timems = timems;
	temp_rcb->tagType = TagType::VideoTag;
	if (temp_rcb->tag_body_isbig)
		app_free(temp_rcb->tag_body_big);//free node:+
	char* dst = nullptr;
	temp_rcb->tag_body_length = length;
	if (length > DefaultItemSize) {
		temp_rcb->tag_body_big = (char*)app_malloc(length + 4);
		dst = temp_rcb->tag_body_big;
		temp_rcb->tag_body_isbig = true;
	}
	else {
		dst = temp_rcb->tag_body_default;
		temp_rcb->tag_body_isbig = false;
	}
	memcpy(dst, data, length);

	//remove h264 nal  00 00 00 01
	char *skip = dst;
	while (*(skip++) != 0x1);
	int skipBytes = (int)(skip - dst);
	temp_rcb->flv_tag_real = dst + skipBytes;
	temp_rcb->flv_tag_real_length = temp_rcb->tag_body_length - skipBytes + 4;//+4 是 PreviousTagSize 注意：不包过视频头

	dst = dst + temp_rcb->tag_body_length;
	EncodeInt32(dst,
		temp_rcb->tag_body_length - skipBytes + 20);

	char* packet = temp_rcb->flv_tag_header;

	//flv tag header
	UINT networkTimestamp = fastHtonl(timems);
	UINT streamID = 0;

	*packet++ = 0x09;
	packet = EncodeInt24(packet, temp_rcb->flv_tag_real_length + 9 - 4);//3byte  +9的是因为视频头

	//time tamp-> [time tamp 3b,time tamp ex 1b]
	memcpy(packet, ((LPBYTE)(&networkTimestamp)) + 1, 3);
	packet += 3;

	memcpy(packet, &networkTimestamp, 1);
	packet += 1;

	// 1 -> 3, ↓
	memcpy(packet, &streamID, 3);
	packet += 3;

	//next video header
	packet[0] = isKeyFrame ? 0x17 : 0x27;
	packet[1] = 1;

	int timeOffset = htonl(compositionTime);
	BYTE *timeOffsetAddr = ((BYTE*)&timeOffset) + 1;

	memcpy(packet + 2, timeOffsetAddr, 3);
	packet += 5;
	EncodeInt32(packet, temp_rcb->flv_tag_real_length - 4);

	temp_rcb->compositionTime = timeOffset;

	temp_rcb->isKeyFrame = isKeyFrame;
	if (isKeyFrame)
		this->lastKeyFrame = temp_rcb;

	if (this->ReadLastWrite == nullptr || this->ReadLastWrite->index < temp_rcb->index)
		this->ReadLastWrite = temp_rcb;
	SetEvent(this->new_event);
}

void RingCacheBuffer::overlay_audio(char * data, int length, DWORD timems)
{

	//EnterCriticalSection(&overlay_cs);
	app_atomic_lock(&this->lock_t, rcb_lock_value);
	RCBItem* temp_rcb = (RCBItem*)this->LastWrite;
	temp_rcb = temp_rcb->next;
	this->LastWrite = temp_rcb;
	temp_rcb->index = this->tag_index++;//
	//LeaveCriticalSection(&overlay_cs);
	app_atomic_unlock(&this->lock_t, rcb_lock_value);

	temp_rcb->timems = timems;
	temp_rcb->tagType = TagType::AudioTag;

	if (temp_rcb->tag_body_isbig)
		app_free(temp_rcb->tag_body_big);//free node:+
	char* dst = nullptr;
	temp_rcb->tag_body_length = length;
	if (length > DefaultItemSize) {
		temp_rcb->tag_body_big = (char*)app_malloc(length + 4);
		dst = temp_rcb->tag_body_big;
		temp_rcb->tag_body_isbig = true;
	}
	else {
		dst = temp_rcb->tag_body_default;
		temp_rcb->tag_body_isbig = false;
	}
	memcpy(dst, data, length);

	temp_rcb->flv_tag_real = dst;
	temp_rcb->flv_tag_real_length = temp_rcb->tag_body_length + 4;

	dst = dst + temp_rcb->tag_body_length;
	EncodeInt32(dst,
		temp_rcb->tag_body_length + 11);

	char* packet = temp_rcb->flv_tag_header;

	//flv tag header
	UINT networkTimestamp = fastHtonl(timems);
	UINT streamID = 0;

	*packet++ = 0x08;
	packet = EncodeInt24(packet, temp_rcb->flv_tag_real_length - 4);//3byte

	//time tamp-> [time tamp 3b,time tamp ex 1b]
	memcpy(packet, ((LPBYTE)(&networkTimestamp)) + 1, 3);
	packet += 3;

	memcpy(packet, &networkTimestamp, 1);
	packet += 1;

	// 1 -> 3, ↓
	memcpy(packet, &streamID, 3);
	packet += 3;

	temp_rcb->isKeyFrame = false;

	if (this->ReadLastWrite == nullptr || this->ReadLastWrite->index < temp_rcb->index)
		this->ReadLastWrite = temp_rcb;
	SetEvent(this->new_event);

}


