#include "stdafx.h"
#include "AppContext.h"

AppContext* AppContext::context = nullptr;

void  DbgPrintW(const char *format, ...) 
{
	char buf[4096], *p = buf;
	va_list args;
	va_start(args, format);
	p += _vsnprintf(p, sizeof buf - 1, format, args);
	va_end(args);
	OutputDebugStringW((LPCWSTR)buf);
}

void  DbgPrintA(const char *format, ...)
{
	char buf[4096], *p = buf;
	va_list args;
	va_start(args, format);
	p += _vsnprintf(p, sizeof buf - 1, format, args);
	va_end(args);
	OutputDebugStringA(buf);
}

void CrashErrorWIN32(const char *text)
{
	char message[1200];// 
	DWORD code = GetLastError();
	HLOCAL LocalAddress = NULL;
	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, code, 0, (LPSTR)&LocalAddress, 0, NULL);

	sprintf(message, "%s LastError:%s", text, (LPSTR)LocalAddress);
	MessageBoxA(NULL, message, "WIN32ERROR", MB_OK);
}

void CrashErrorMessage(const char *text)
{
	MessageBoxA(NULL, text, "Error", MB_OK);
}

void* app_malloc(size_t size)
{
	void * p = malloc(size);
	if (p == NULL)
	{
		CrashErrorMessage("malloc memory error,application exit!!");
		exit(0);
	}
	return p;
}

void app_free(void* data)
{
	free(data);
}

void AppWSAStartup()
{
	WSADATA wsaData;
	if (WSAStartup(0x202, &wsaData) != 0)
	{
		CrashErrorWIN32("WSAStartup");
	}
}

QWORD GetQPCTimeNS()
{
	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);

	double timeVal = double(currentTime.QuadPart);
	timeVal *= 1000000000.0;
	timeVal /= double(clockFreq.QuadPart);

	return QWORD(timeVal);
};

QWORD GetQPCTimeMS()
{
	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);

	QWORD timeVal = currentTime.QuadPart;
	timeVal *= 1000;
	timeVal /= clockFreq.QuadPart;

	return timeVal;
}

bool  SleepToNS(QWORD qwNSTime)
{
	QWORD t = GetQPCTimeNS();

	if (t >= qwNSTime)
		return false;

	unsigned int milliseconds = (unsigned int)((qwNSTime - t) / 1000000);
	if (milliseconds > 1)
	{
		Sleep(milliseconds );
	}
	else
	{
		return true;
	}

	for (;;)
	{
		t = GetQPCTimeNS();
		if (t >= qwNSTime)
			return true;
		Sleep(1);
	}
}

AppContext::AppContext()
{

	AppWSAStartup();
	QueryPerformanceFrequency(&clockFreq);
}

AppContext::~AppContext()
{

}

void AppContext::Log(int level, wchar_t* logtext)
{

}

void AppContext::SetVideoSize(int width, int height)
{
	this->video_width = width;
	this->video_height = height;
}

int AppContext::GetWidth()
{
	return this->video_width;
}

int AppContext::GetHeight()
{
	return this->video_height;
}