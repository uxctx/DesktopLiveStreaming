#pragma once
#include "AppContext.h"

#define IO_CONTEXT_BUFF_SIZE (4*1024)
#define SERVER_THREAD_NUM (1)

enum io_context_preation
{
	io_accept,
	io_revc,
	io_write
};

enum io_context_status
{
	IDLE,
	WILL_CLOSE,
	WORK,
};

typedef struct _io_context {
	OVERLAPPED               Overlapped;
	char                        buffer[IO_CONTEXT_BUFF_SIZE];
	WSABUF                      wsabuf;
	io_context_preation         operation;
	SOCKET                      socket;
	SOCKET                      appect;//Only Accept OPERATION
	UINT32						recv_length;
	LPFN_TRANSMITPACKETS fnTransmitPackets;//TransmitPackets function
	void* connection;
} io_context, *p_io_context;

enum bind_listen_code
{
	port_occupation,
	fatal_error,
	success
};

void free_context(p_io_context context);

typedef void (*iocp_event)(p_io_context iocontext);

void post_wsarecv(p_io_context iocontext);

UINT _stdcall StartIocpServer(LPVOID lParam);

class IOCPServer
{

private:
	iocp_event event_hander;
	int port;
	HANDLE iocp;
	HANDLE thread;
	SOCKET listen_socket;
	p_io_context iocontext;
	bool run;
	bool run_state;
	//io_context accpet_iocontexts[POST_ACCEOT_COUNT];

	HANDLE threads[SERVER_THREAD_NUM];

	void server_work();
public:
	IOCPServer(int port ,iocp_event event_hander);
	~IOCPServer();
	bind_listen_code bind_listen();
	bool start();
	void stop();

	friend UINT _stdcall StartIocpServer(LPVOID lParam);
};

