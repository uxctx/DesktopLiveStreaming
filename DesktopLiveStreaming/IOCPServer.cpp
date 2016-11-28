#include "IOCPServer.h"

GUID acceptex_guid = WSAID_ACCEPTEX;
LPFN_ACCEPTEX	fnAcceptEx;

GUID transmitPackets_guid = WSAID_TRANSMITPACKETS;

SOCKET create_socket(void) {
	int nRet = 0;
	int nZero = 0;
	SOCKET sdSocket = INVALID_SOCKET;

	sdSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (sdSocket == INVALID_SOCKET) {
		return sdSocket;
	}

	//设置为0，关闭发送缓冲区
	nZero = 0;
	nRet = setsockopt(sdSocket, SOL_SOCKET, SO_SNDBUF, (char *)&nZero, sizeof(nZero));
	if (nRet == SOCKET_ERROR) {
		return(sdSocket);
	}
	//关闭 Nagle算法
	const char chOpt = 1;
	nRet = setsockopt(sdSocket, IPPROTO_TCP, TCP_NODELAY, &chOpt, sizeof(char));

	//LINGER lingerStruct;

	//lingerStruct.l_onoff = 1;
	//lingerStruct.l_linger = 0;
	//nRet = setsockopt(sdSocket, SOL_SOCKET, SO_LINGER,
	//	(char *)&lingerStruct, sizeof(lingerStruct));
	//if (nRet == SOCKET_ERROR) {
	//}

	return(sdSocket);
}

p_io_context create_context(SOCKET socket)
{
	p_io_context context = (p_io_context)heap_malloc(sizeof(io_context));
	context->socket = socket;
	context->appect = INVALID_SOCKET;

	context->wsabuf.buf = context->buffer;
	context->wsabuf.len = sizeof(context->buffer);
	context->operation = io_context_preation::io_accept;
	return context;
};

void free_context(p_io_context context)
{
	if (context != nullptr)
		heap_free(context);
}

UINT _stdcall  StartIocpServer(LPVOID lParam)
{
	((IOCPServer*)lParam)->server_work();
	return 0;
};

p_io_context update_completion_port(SOCKET socket, HANDLE iocp)
{
	p_io_context io_context = create_context(socket);
	HANDLE result = CreateIoCompletionPort((HANDLE)socket, iocp, (DWORD_PTR)io_context, 0);
	if (result == NULL) {
		free_context(io_context);
		return NULL;
	}
	return io_context;
}

int post_acceptEx(p_io_context io_context)
{
	io_context->appect = create_socket();
	io_context->operation = io_context_preation::io_accept;
	static DWORD dwRecvNumBytes = 0;
	return AcceptEx(io_context->socket,
		io_context->appect,
		(LPVOID)(io_context->buffer),
		//MAX_BUFF_SIZE - (2 * (sizeof(SOCKADDR_STORAGE) + 16)),
		0,
		sizeof(SOCKADDR_STORAGE) + 16,
		sizeof(SOCKADDR_STORAGE) + 16,
		&dwRecvNumBytes,
		(LPOVERLAPPED) &(io_context->Overlapped));
};

void post_wsarecv(p_io_context iocontext)
{
	static DWORD dwRecvNumBytes = 0;
	static DWORD dwSendNumBytes = 0;
	static	DWORD dwFlags = 0;
	
	iocontext->operation = io_context_preation::io_revc;
	int ret = WSARecv(
		iocontext->socket,
		&iocontext->wsabuf,
		1,
		&dwRecvNumBytes,
		&dwFlags,
		&iocontext->Overlapped, NULL);
	
	if (ret == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()))
	{
		DbgPrintA("[ERROR] post_wsarecv: %d,ret: %d \n", iocontext->socket, ret);
	}
	DbgPrintA("post_wsarecv: %d,ret: %d \n", iocontext->socket, ret);
}

IOCPServer::IOCPServer(int port, iocp_event event_hander)
{
	this->port = port;
	this->event_hander = event_hander;
	run_state = false;
}

IOCPServer::~IOCPServer()
{
	//？
	CloseHandle(this->iocp);
	closesocket(this->listen_socket);
	free_context(iocontext);
}

bind_listen_code IOCPServer::bind_listen()
{
	this->iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (this->iocp == NULL) {
		CrashErrorWIN32("CreateIoCompletionPort");
		return bind_listen_code::fatal_error;
	}
	sockaddr_in  hints;
	hints.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	hints.sin_family = AF_INET;
	hints.sin_port = htons(this->port);

	int ret = 0, zero = 0;

	this->listen_socket = create_socket();

	ret = bind(this->listen_socket, (SOCKADDR*)&hints, sizeof(SOCKADDR));
	if (ret == WSAEADDRINUSE) {
		CloseHandle(this->iocp);
		closesocket(this->listen_socket);
		return bind_listen_code::port_occupation;
	}
	if (ret != 0) {
		CrashErrorWIN32("可能端口被占用");
		goto goto_fatal_error;
	}

	ret = listen(this->listen_socket, 8);
	if (ret == SOCKET_ERROR) {
		CrashErrorWIN32("listen");
		goto goto_fatal_error;
	}
	iocontext = update_completion_port(this->listen_socket, this->iocp);
	if (iocontext == NULL) {
		CrashErrorWIN32("UpdateCompletionPort");
		goto goto_fatal_error;
	}
	
	ret = post_acceptEx(iocontext);

	return bind_listen_code::success;

goto_fatal_error:
	CloseHandle(this->iocp);
	closesocket(this->listen_socket);
	free_context(iocontext);
	return bind_listen_code::fatal_error;
}

bool IOCPServer::start()
{
	if (run_state)
		return false;
	this->run = true;
	for (size_t i = 0; i < SERVER_THREAD_NUM; i++)
	{
		threads[i] = (HANDLE)_beginthreadex(NULL, 0, StartIocpServer, this, 0, NULL);
	}
	run_state = true;
	return false;
}

void IOCPServer::stop()
{
	this->run = false;
	PostQueuedCompletionStatus(this->iocp, 0, 0, NULL);
	WaitForMultipleObjects(SERVER_THREAD_NUM, threads, TRUE, INFINITE);
	run_state = false;
}

void IOCPServer::server_work()
{
	DWORD dwFlags = 0;
	DWORD dwIoSize = 0;
	p_io_context queued_io;
	LPWSAOVERLAPPED lp_overlapped = NULL;

	p_io_context accept_context;

	DWORD bytes = 0;
	int ret;
	while (this->run)
	{
		if (GetQueuedCompletionStatus(
			iocp,
			&dwIoSize,
			(PDWORD_PTR)&queued_io,
			(LPOVERLAPPED *)&lp_overlapped,
			INFINITE
		))
		{
			if (lp_overlapped == nullptr){
				this->run = false;
				break;
			}
			else
			{
				switch (queued_io->operation)
				{
				case io_context_preation::io_accept:

					DbgPrintA("io_accept: %d \n", queued_io->appect);

					 accept_context = update_completion_port(queued_io->appect, iocp);
					post_acceptEx(iocontext);
					ret = WSAIoctl(
						accept_context->socket,
						SIO_GET_EXTENSION_FUNCTION_POINTER,
						&transmitPackets_guid,
						sizeof(transmitPackets_guid),
						&accept_context->fnTransmitPackets,
						sizeof(accept_context->fnTransmitPackets),
						&bytes,
						NULL,
						NULL
					);
					if (ret == SOCKET_ERROR) {
						CrashErrorWIN32("WSAIoctl TransmitPackets Error!");
					}
					this->event_hander(accept_context);
					break;
				case io_context_preation::io_revc:
					DbgPrintA("io_revc: %d size:%d \n", queued_io->socket, dwIoSize);
					queued_io->recv_length = dwIoSize;
					this->event_hander(queued_io);
					break;
				case io_context_preation::io_write:
					DbgPrintA("io_write: %d size:%d \n", queued_io->socket, dwIoSize);

					this->event_hander(queued_io);
					break;
				default:
					break;
				}		
			}
			continue;
		}
		else
		{
			int code = WSAGetLastError();
			if (code == ERROR_NETNAME_DELETED)//表示该socket在客户端被强制关闭了
			{
				DbgPrintA("[ERROR_NETNAME_DELETED]: %d \n", queued_io->socket);
				queued_io->operation = io_context_preation::io_revc;
				queued_io->recv_length = 0;//投递出去，让外层关闭
				this->event_hander(queued_io);
				continue;
			}
			CrashErrorWIN32("GetQueuedCompletionStatus Error!");
			this->run = false;
			break;
		}
	}
}