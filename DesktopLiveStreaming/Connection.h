#pragma once
#include "AppContext.h"
#include "IOCPServer.h"
#include "http_parser.h"
#include "ResponseHeaderText.h"
#include "RingCacheBuffer.h"

#define MAXURL (256)

enum res_context_type{
	html,//text/html
	js,//application/x-javascript
	css,//	text/css
	swf,//application/x-shockwave-flash

	txt,//text/plain

	m3u8,//audio/x-mpegurl
	ts,//video/mp2t

	ico,//image/x-icon
	jpg,//image/jpeg
};

 struct static_res {
	char url[128];
	wchar_t file[128];
	res_context_type type;
};

 enum connet_state {
	 flv_just_connect,//next send flvheader
	 flv_push_stream,//send tag data
	 flv_will_be_close,
 };

 enum connet_type {
	 http_con,
	 flv_stream_con,
 };

 enum connet_iostate {
	 idle,
	 sending,
 };

 
class Connection
{
private:
	p_io_context iocontext;
	http_parser parser;
	http_parser_settings settings;
	char url[MAXURL];

	char cp_content_length[40];//Content-Length:
	HANDLE work_send_hFile;

	Connection *next;
	connet_state state;
	connet_iostate iostate;
	connet_type type;

	TRANSMIT_PACKETS_ELEMENT transmit_packets_elements[4];

	char flv_tag_header[20];// size 11+9固定大小 11是tag头 9是视频头 音频也用这个字段
	RCBItem* last_send;
	unsigned int cr_index;
	QWORD  just_timems;

	void recv_hander();
	void write_hander();
	
	//[delete this!!!]
	void close_connet();
public:
	Connection(p_io_context iocontext);
	~Connection();

	void do_event();

	friend class FlvLiveStream;
	friend void send_static_file(Connection *con, p_io_context iocontext, char* url);
	friend void  send_m3u8(Connection *con, p_io_context iocontext);
	friend void  send_livemodel(Connection *con, p_io_context iocontext);
};

int get_context_type_str(res_context_type type, char** typestr);

void send_static_file(Connection *con, p_io_context iocontext, char* url);

void event_hander(p_io_context iocontext);

void init_res_table();

void send_m3u8(Connection *con, p_io_context iocontext); 

void send_livemodel(Connection *con, p_io_context iocontext);