#include "stdafx.h"
#include "CommonFun.h"

//threads access unsafe£¿

void CrashError(HRESULT code, const wchar_t* text)
{
	wchar_t message[1200];// 
	wsprintfW(message, L"%s Error,result:%ld", text, code);
	MessageBox(NULL, message, L"Error", MB_OK);
}