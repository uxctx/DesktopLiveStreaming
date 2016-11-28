#include "Connection.h"
#include "uthash.h"

#include <shlwapi.h>
#include "HLS_Server.h"

#define static_dir "./web_static"
#define error_url "./error.html"
#define index_url "./error.html"

struct ExtensionHash {
	char* extension;
	res_context_type type;
	UT_hash_handle hh;
};

static ExtensionHash extensions_data[20];
static ExtensionHash *head = NULL;

#define  extensions_hash_set(exstr,enumtype) 		 \
		set=&extensions_data[index++];			 \
		set->extension = exstr;					 \
		set->type = enumtype;						 \
		HASH_ADD_STR(head, extension, set)	 

void init_res_table()
{

	int index = 0;
	ExtensionHash *set = nullptr;

	extensions_hash_set(".html", res_context_type::html);
	extensions_hash_set(".js", res_context_type::js);
	extensions_hash_set(".css", res_context_type::css);
	extensions_hash_set(".swf", res_context_type::swf);
	extensions_hash_set(".txt", res_context_type::txt);

	extensions_hash_set(".m3u8", res_context_type::m3u8);
	extensions_hash_set(".ts", res_context_type::ts);

	extensions_hash_set(".ico", res_context_type::ico);
	extensions_hash_set(".jpg", res_context_type::jpg);

}

void send_livemodel(Connection *con, p_io_context iocontext)
{
	char* dst = iocontext->buffer;
	int bodylen = 0, tmplen = 0;
	char *httpflv = "{\"mode\":\"httpflv\",\"url\":\""
		FLVLIVEURL
		"\"}";
	char *hlsflv = "{\"mode\":\"hls\",\"url\":\""
		HLSM3U8URL
		"\"}";
	bodylen = strlen(strcpy(dst, 
		MyContext->live_model == LiveModel::http_flv ? httpflv : hlsflv));

	LPTRANSMIT_PACKETS_ELEMENT lpe = con->transmit_packets_elements;
	lpe[0].dwElFlags = TP_ELEMENT_MEMORY;
	lpe[0].cLength = response_json_size;
	lpe[0].pBuffer = (void*)response_json;

	tmplen = sprintf(con->cp_content_length, "Content-Length:%d\r\n\r\n", bodylen);
	lpe[1].dwElFlags = TP_ELEMENT_MEMORY;
	lpe[1].cLength = tmplen;
	lpe[1].pBuffer = con->cp_content_length;


	lpe[2].dwElFlags = TP_ELEMENT_MEMORY;
	lpe[2].cLength = bodylen;
	lpe[2].pBuffer = iocontext->buffer;

	iocontext->operation = io_context_preation::io_write;
	if (!iocontext->fnTransmitPackets(iocontext->socket,
		lpe,
		3,
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
			CrashErrorWIN32("send_m3u8 TransmitPackets Error!");
		}

		//DbgPrintA("TransmitPackets: %d \n", iocontext->socket);
	}
}

void send_m3u8(Connection *con, p_io_context iocontext)
{

	char* dst = iocontext->buffer;
	int bodylen = 0, tmplen = 0;
	bodylen = MyContext->hlsServer->get_m3u8_data(dst);

	LPTRANSMIT_PACKETS_ELEMENT lpe = con->transmit_packets_elements;
	lpe[0].dwElFlags = TP_ELEMENT_MEMORY;
	lpe[0].cLength = response_m3u8_size;
	lpe[0].pBuffer = (void*)response_m3u8;

	tmplen = sprintf(con->cp_content_length, "Content-Length:%d\r\n\r\n", bodylen);
	lpe[1].dwElFlags = TP_ELEMENT_MEMORY;
	lpe[1].cLength = tmplen;
	lpe[1].pBuffer = con->cp_content_length;

	lpe[2].dwElFlags = TP_ELEMENT_MEMORY;
	lpe[2].cLength = bodylen;
	lpe[2].pBuffer = iocontext->buffer;

	iocontext->operation = io_context_preation::io_write;
	if (!iocontext->fnTransmitPackets(iocontext->socket,
		lpe,
		3,
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
			CrashErrorWIN32("send_m3u8 TransmitPackets Error!");
		}

		//DbgPrintA("TransmitPackets: %d \n", iocontext->socket);
	}
}

void send_static_file(Connection *con, p_io_context iocontext, char* url)
{
	char filename[256];
	if (strcmp(url, "/") == 0) {
		sprintf(filename, "%s%s", static_dir, "/index.html");
	}
	else if (url[0] == '/'&&
		url[1] == 'h'&&
		url[2] == 'l'&&
		url[3] == 's'&&
		url[4] == '/') {
		sprintf(filename, ".%s", url);
	}
	else {
		sprintf(filename, "%s%s", static_dir, url);
	}

	con->work_send_hFile = CreateFileA(filename,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		NULL,
		NULL);

	if (con->work_send_hFile == INVALID_HANDLE_VALUE) {
		send_static_file(con, iocontext, error_url);
		return;
	}

	char* exten = PathFindExtensionA(filename);

	void* response_header = NULL;
	size_t response_header_size = 0;

	LPTRANSMIT_PACKETS_ELEMENT lpe = con->transmit_packets_elements;

	lpe[0].dwElFlags = TP_ELEMENT_MEMORY;
	ExtensionHash *tmp;
	HASH_FIND_STR(head, exten, tmp);
	res_context_type tmptype = tmp == NULL ? html : tmp->type;
	switch (tmptype)
	{
	case html:
		response_header_size = response_html_size;
		response_header = (void*)response_html;
		break;
	case js:
		response_header_size = response_js_size;
		response_header = (void*)response_js;
		break;
	case css:
		break;
	case swf:
		response_header_size = response_swf_size;
		response_header = (void*)response_swf;
		break;
	case txt:
		break;
	case m3u8:
		response_header_size = response_m3u8_size;
		response_header = (void*)response_m3u8;
		break;
	case ts:
		response_header_size = response_ts_size;
		response_header = (void*)response_ts;
		break;
	case ico:
		response_header_size = response_ico_size;
		response_header = (void*)response_ico;
		break;
	case jpg:
		response_header_size = response_jpg_size;
		response_header = (void*)response_jpg;
		break;
	default:
		response_header_size = response_stream_size;
		response_header = (void*)response_stream;
		break;
	}
	lpe[0].cLength = response_header_size;
	lpe[0].pBuffer = response_header;

	int tmlen = GetFileSize(con->work_send_hFile, 0);
	tmlen = sprintf(con->cp_content_length, "Content-Length:%d\r\n\r\n", tmlen);
	lpe[1].dwElFlags = TP_ELEMENT_MEMORY;
	lpe[1].cLength = tmlen;
	lpe[1].pBuffer = con->cp_content_length;

	lpe[2].dwElFlags = TP_ELEMENT_FILE;
	lpe[2].cLength = 0;

	lpe[2].hFile = con->work_send_hFile;

	lpe[2].nFileOffset = { 0 };

	iocontext->operation = io_context_preation::io_write;
	if (!iocontext->fnTransmitPackets(iocontext->socket,
		lpe,
		3,
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

		DbgPrintA("TransmitPackets: %d \n", iocontext->socket);
	}
}