#pragma once
#include "AppContext.h"
#include "H264_Encoder.h"
#include "AudioSoureAACStream.h"
#include "Connection.h"

struct ts_file {
	char filename[32];
	char url[32];
	size_t sequence;
	float extinf;
};

static u_char   aud_nal[] = { 0x00, 0x00, 0x00, 0x01, 0x09, 0xf0 };

#define h264_frame_size  (1024 * 1024)
#define aac_frame_size 1024

#define m3u8_ts_list_count (3)
#define m3u8_ts_duration (5) //5s

#define m3u8_list_lock (1)
#define m3u8_list_lock_read (2)

static wchar_t *hls_cache_path = L".\\hls\\";

UINT _stdcall TsFactoryWork(LPVOID lParam);
class HLS_Server
{
private:
	bool run;
	HANDLE thread[1];
	RingCacheBuffer* rcb;
	H264_Encoder* h264;
	char m3u8_data[1024 * 2];
	char m3u8_data_copy[1024*2];
	size_t m3u8_data_len;

	app_atomic_lock_t m3u8_data_lock;

	void work();
public:
	HLS_Server();
	~HLS_Server();

	void start();
	void stop();

	int get_m3u8_data(char* dst);

	friend UINT _stdcall TsFactoryWork(LPVOID lParam);
	//friend void send_m3u8(Connection *con, p_io_context iocontext);
};

