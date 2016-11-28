#include "stdafx.h"
#include "Connection.h"
#include "FlvLiveStream.h"


int get_context_type_str(res_context_type type, char** typestr)
{
	//下面的字符串编译器会优化
	switch (type)
	{
	case html:
		*typestr = "text/html";
		return strlen("text/html");
		break;
	case js:
		*typestr = "application/x-javascript";
		return strlen("application/x-javascript");
		break;
	case swf:
		*typestr = "application/x-shockwave-flash";
		return strlen("application/x-shockwave-flash");
		break;
	case txt:
		*typestr = "text/plain";
		return strlen("text/plain");
		break;
	case m3u8:
		*typestr = "audio/x-mpegurl";
		return strlen("audio/x-mpegurl");
		break;
	case ts:
		*typestr = "video/mp2t";
		return strlen("video/mp2t");
		break;
	default:
		break;
	}
}

void Connection::recv_hander()
{
	if (iocontext->recv_length <= 0)
	{
		if(this->type == connet_type::flv_stream_con)
			this->state = connet_state::flv_will_be_close;
		else
		{
			goto recv_error_close;
		}
		return;
	}

	size_t nparsed = http_parser_execute(
		&this->parser,
		&settings,
		iocontext->buffer,
		iocontext->recv_length);
	if (nparsed == iocontext->recv_length &&
		!this->parser.upgrade)
	{

	}
	else
	{
		strcpy(this->url, "/error");
	}
	DbgPrintA("[http_request] socket: %d url: %s \n", this->iocontext->socket,
		this->url);
	if (MyContext->live_model== LiveModel::http_flv&&
		strcmp(FLVLIVEURL, this->url) == 0)
	{
		this->iostate = connet_iostate::idle;
		this->type = connet_type::flv_stream_con;
		this->state = connet_state::flv_just_connect;
		MyContext->flvServer->add_connection(this);
		return;
	}
	if (MyContext->live_model == LiveModel::hls&&
		strcmp(HLSM3U8URL, this->url) == 0)
	{
		send_m3u8(this, iocontext);
		return;
	}
	if (strcmp(GETMODEL, this->url) == 0)
	{
		send_livemodel(this, iocontext);
		return;
	}

	send_static_file(this, iocontext, this->url);
	return;
recv_error_close:
	if (this->type == connet_type::flv_stream_con) {
		this->state = connet_state::flv_will_be_close;
		return;
	}
	this->close_connet();
	return;
}

void Connection::write_hander()
{
	
	if (this->type == connet_type::flv_stream_con) {

		this->iostate = connet_iostate::idle;
	}
	else
	{
		if (this->work_send_hFile != NULL) {
			CloseHandle(this->work_send_hFile);
			this->work_send_hFile = NULL;
		}
		this->close_connet();//暂时拒绝 http keep-alive 程序设计有缺陷

		return;
		//post_wsarecv(iocontext);
	}
		
	if (this->work_send_hFile != NULL) {
		CloseHandle(this->work_send_hFile);
		this->work_send_hFile = NULL;
	}
}

void Connection::close_connet()
{
	closesocket(iocontext->socket);
	free_context(iocontext);
	DbgPrintA("[recv_close]: %d \n", iocontext->socket);
	delete this;
}

Connection::Connection(p_io_context iocontext)
{
	this->type == connet_type::http_con;
	this->iocontext = iocontext;
	this->work_send_hFile == NULL;
	http_parser_init(&parser, HTTP_REQUEST);
	memset(&settings, 0, sizeof(http_parser_settings));
	parser.data = this->url;
	settings.on_url =
		[](http_parser* parser, const char *at, size_t length) -> int {
		if (length > MAXURL)
			strcpy((char*)parser->data, "/error");
		else
		{
			memcpy((char*)parser->data, at, length);
			((char*)parser->data)[length] = '\0';
		}
		return 0;
	};
	strcpy(cp_content_length, "Content-Length:");
}

Connection::~Connection()
{

}

void Connection::do_event()
{
	switch (this->iocontext->operation)
	{
	case io_context_preation::io_accept:
		post_wsarecv(iocontext);
		break;
	case io_context_preation::io_revc:
		this->recv_hander();
		break;
	case io_context_preation::io_write:
		this->write_hander();
		break;
	default:
		break;
	}
}

void event_hander(p_io_context iocontext)
{
	if (iocontext->operation == io_context_preation::io_accept)
		iocontext->connection = new Connection(iocontext);

	((Connection*)(iocontext->connection))->do_event();

}
