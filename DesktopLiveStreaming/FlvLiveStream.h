#pragma once
#include "Connection.h"

UINT _stdcall FlvLiveWork(LPVOID lParam);
class FlvLiveStream
{
private:
	bool run;
	HANDLE thread[1];
	CRITICAL_SECTION head_cs;
	Connection *head;
	RingCacheBuffer *rcb;
	char flv_header_tag[600];
	int flv_header_tag_len;

	void add_connection(Connection *con);
	void work();

	void send_header(Connection *work);
	void send_transmit_tags(Connection *work);
public:
	FlvLiveStream();
	~FlvLiveStream();

	void start();
	void stop();

	
	friend class Connection;
	friend UINT _stdcall FlvLiveWork(LPVOID lParam);
};

