#pragma once
#include "AppContext.h"

//程序太小，没必要写头部动态组合了，全部写死了，

#define RESPONSE_HEADER_CONCLOSE "Connection: close\r\n"
#define RESPONSE_HEADER_KEEPALIVE "Connection:keep-alive\r\n"

#define RESPONSE_HEADER_NOCACHE "Cache-Control: no-cache\r\n"\
								"Pragma: no-cache\r\n"

#define RESPONSE_HEADER_START "HTTP/1.1 200 OK\r\n"\
				"Server: DesktopLiveStreaming\r\n"\

#define RESPONSE_HEADER_Allow_Origin	"Access-Control-Allow-Origin: *\r\n"

static const char *response_flv = RESPONSE_HEADER_START
RESPONSE_HEADER_NOCACHE
RESPONSE_HEADER_CONCLOSE
RESPONSE_HEADER_Allow_Origin
"Content-Type: video/x-flv\r\n"
"\r\n";
static const size_t response_flv_size = strlen(response_flv);

static const char *response_html = RESPONSE_HEADER_START
RESPONSE_HEADER_NOCACHE
RESPONSE_HEADER_CONCLOSE
RESPONSE_HEADER_Allow_Origin
"Content-Type: text/html\r\n";
static const size_t response_html_size = strlen(response_html);

static const char *response_js = RESPONSE_HEADER_START
RESPONSE_HEADER_NOCACHE
RESPONSE_HEADER_CONCLOSE
"Content-Type: application/x-javascript\r\n";
static const size_t response_js_size = strlen(response_js);

static const char *response_swf = RESPONSE_HEADER_START
RESPONSE_HEADER_Allow_Origin
RESPONSE_HEADER_CONCLOSE
RESPONSE_HEADER_NOCACHE
"Content-Type: application/x-shockwave-flash\r\n";
static const size_t response_swf_size = strlen(response_swf);

static const char *response_ico = RESPONSE_HEADER_START
"Content-Type: image/x-icon\r\n";
static const size_t response_ico_size = strlen(response_ico);

static const char *response_jpg = RESPONSE_HEADER_START
"Content-Type: image/jpeg\r\n";
static const size_t response_jpg_size = strlen(response_jpg);

static const char *response_m3u8 = RESPONSE_HEADER_START
RESPONSE_HEADER_Allow_Origin
RESPONSE_HEADER_NOCACHE
//"Content-Type: application/vnd.apple.mpegurl\r\n";
"Content-Type: text/plain\r\n";
static const size_t response_m3u8_size = strlen(response_m3u8);

static const char *response_ts = RESPONSE_HEADER_START
RESPONSE_HEADER_Allow_Origin
RESPONSE_HEADER_NOCACHE
"Content-Type: video/MP2T\r\n";
static const size_t response_ts_size = strlen(response_ts);

static const char *response_stream = RESPONSE_HEADER_START
RESPONSE_HEADER_Allow_Origin
RESPONSE_HEADER_NOCACHE
"Content-Type: application/octet-stream\r\n";
static const size_t response_stream_size = strlen(response_stream);

static const char *response_json = RESPONSE_HEADER_START
RESPONSE_HEADER_NOCACHE
RESPONSE_HEADER_Allow_Origin
"Content-Type: application/json\r\n";
static const size_t response_json_size = strlen(response_json);

static const char *response_CRLF = "\r\n\r\n";
static const size_t response_CRLF_size = strlen(response_CRLF);