#include "stdafx.h"
#include "FlvLiveStream.h"
#include "AppContext.h"
#include "H264_Encoder.h"
#include "AudioSoureAACStream.h"
#include "FLVhelp.h"

void FlvLiveStream::add_connection(Connection * con)
{
	EnterCriticalSection(&head_cs);
	if (this->head == nullptr)
		this->head = con;
	else
	{
		con->next = this->head;
		this->head = con;
	}
	LeaveCriticalSection(&head_cs);
}

void FlvLiveStream::work()
{
	Connection *work_head;
	while (this->run)
	{
		if (this->head == nullptr) {
			Sleep(200);
			continue;
		}

		WaitForSingleObject(this->rcb->new_event, INFINITE);

		EnterCriticalSection(&head_cs);
		work_head = this->head;
		while (work_head != nullptr&&work_head->state ==
			connet_state::flv_will_be_close)
		{
			work_head = this->head->next;
			this->head->close_connet();
			this->head = work_head;
		}
		LeaveCriticalSection(&head_cs);

		Connection *tempwork_head;
		while (work_head != nullptr) {
			switch (work_head->state)
			{
			case flv_just_connect:
				this->send_header(work_head);
				work_head = work_head->next;
				break;
			case flv_push_stream:
				this->send_transmit_tags(work_head);
				work_head = work_head->next;
				break;
			case flv_will_be_close:
				tempwork_head = work_head;
				work_head = work_head->next;//必要在前
				tempwork_head->close_connet();
				break;
			default:
				break;
			}
		}
	}
}

void FlvLiveStream::send_header(Connection *work)
{
	work->state = connet_state::flv_push_stream;
	p_io_context iocontext = work->iocontext;
	iocontext->wsabuf.buf = this->flv_header_tag;
	iocontext->wsabuf.len = this->flv_header_tag_len;
	static DWORD dwSendNumBytes = 0;
	static DWORD dwFlags = 0;
	work->iostate = connet_iostate::sending;
	iocontext->operation = io_context_preation::io_write;

	int ret = WSASend(work->iocontext->socket,
		&iocontext->wsabuf,
		1,
		&dwSendNumBytes,
		dwFlags,
		&(iocontext->Overlapped),
		NULL);
}

void FlvLiveStream::send_transmit_tags(Connection *work)
{
	if (work->iostate != connet_iostate::idle)
		return;
	RCBItem* nowsend;
	if (work->last_send == nullptr)
	{
		nowsend = this->rcb->lastKeyFrame;//从关键帧开始
	}
	else
	{
		if (work->last_send->index == this->rcb->ReadLastWrite->index)
			return;
		nowsend = work->last_send->next;
	}

	memcpy(work->flv_tag_header, nowsend->flv_tag_header, 20);

	DWORD timestamp = 0;

	if (work->just_timems == 0)
	{
		work->just_timems = nowsend->timems;
		timestamp = 0;
	}
	else
	{
		timestamp = nowsend->timems - work->just_timems;
	}

	DbgPrintA("[transmit_tags] index %d  type: %d \n ", nowsend->index, nowsend->tagType);
	UINT networkTimestamp = fastHtonl(timestamp);

	//time tamp-> [time tamp 3b,time tamp ex 1b]
	// TYPE 1  + Size 3 + timestamp 4 +streamID 3 
	char *timestamp_data = ((char*)work->flv_tag_header) + 4;
	memcpy(timestamp_data, ((LPBYTE)(&networkTimestamp)) + 1, 3);
	memcpy(timestamp_data + 3, &networkTimestamp, 1);

	LPTRANSMIT_PACKETS_ELEMENT lpe = work->transmit_packets_elements;
	lpe[0].dwElFlags = TP_ELEMENT_MEMORY;
	lpe[0].cLength = nowsend->tagType == TagType::VideoTag ? 20 : 11;
	lpe[0].pBuffer = work->flv_tag_header;


	lpe[1].dwElFlags = TP_ELEMENT_MEMORY;
	lpe[1].cLength = nowsend->flv_tag_real_length;
	lpe[1].pBuffer = nowsend->flv_tag_real;
	work->last_send = nowsend;
	p_io_context iocontext = work->iocontext;
	iocontext->operation = io_context_preation::io_write;
	work->iostate = connet_iostate::sending;
	if (!iocontext->fnTransmitPackets(iocontext->socket,
		lpe,
		2,
		0,
		&iocontext->Overlapped,
		TF_USE_DEFAULT_WORKER))
	{
		DWORD code = GetLastError();
		if (code == ERROR_IO_PENDING || code == WSA_IO_PENDING)
		{
		}
		else
		{
			CrashErrorWIN32("TransmitPackets Error!");
		}

		//DbgPrintA("TransmitPackets: %d \n", iocontext->socket);
	}

}

FlvLiveStream::FlvLiveStream()
{
	InitializeCriticalSection(&head_cs);
	this->head = nullptr;

	this->rcb = MyContext->rCache;
	char h264_header[80];
	int h264_header_len;
	h264_header_len = MyContext->h264Encoder->get_sps_pps(h264_header);

	char acc_header[8];
	int acc_header_len;

	int width, height;
	double	audiodatarate, audiosamplerate, audiosamplesize, audiochannels;

	PWAVEFORMATEX fmt = nullptr;
	if (AppContext::context->accSource->can_work)
	{
		fmt = AppContext::context->accSource->GetFmt();
		audiosamplerate = fmt->nSamplesPerSec;
		audiochannels = fmt->nChannels;
	}
	else
	{
		audiosamplerate = 48000;
		audiochannels = 2;
	}
	width = AppContext::context->GetWidth();
	height = AppContext::context->GetHeight();

	audiodatarate = AACbitRate;

	audiosamplesize = 16.0;//？

	char *dst = this->flv_header_tag;

	memcpy(dst, response_flv, response_flv_size);
	dst += response_flv_size;
	dst = FLVhelp::HeaderAndMetaDataTag(dst,
		width, height,
		audiodatarate, audiosamplerate, audiosamplesize, audiochannels);
	dst = FLVhelp::FLVPacket(dst, h264_header, h264_header_len, 9, 0);

	if (MyContext->accSource->can_work)
	{
		acc_header_len = MyContext->accSource->GetHeander(acc_header);
		dst = FLVhelp::FLVPacket(dst, acc_header, acc_header_len, 8, 0);
	}

	this->flv_header_tag_len = dst - this->flv_header_tag;

}

FlvLiveStream::~FlvLiveStream()
{
	DeleteCriticalSection(&head_cs);
}

void FlvLiveStream::start()
{
	this->run = true;
	thread[0] = (HANDLE)_beginthreadex(NULL, 0, FlvLiveWork, this, 0, NULL);

}

void FlvLiveStream::stop()
{
	this->run = false;
	WaitForMultipleObjects(1, thread, TRUE, INFINITE);
	CloseHandle(thread[0]);
}

UINT _stdcall FlvLiveWork(LPVOID lParam)
{
	((FlvLiveStream*)lParam)->work();
	return 0;
}
