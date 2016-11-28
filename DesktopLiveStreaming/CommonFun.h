#pragma once
#ifndef _COMMONFUN
#define _COMMONFUN


#include "stdafx.h"
#include <stdio.h>

#include "VertexShader.h"
#include "PixelShader.h"

#define RELEASE(__p) {if(__p!=nullptr){__p->Release();__p=nullptr;}}

#define HRCHECKTOEND(hr,text) {if(FAILED(hr)){CrashError(hr,text);goto error;}}
#define HRCHECK(hr,text) {if(FAILED(hr)){CrashError(hr,text);}}

void CrashError(HRESULT code, const wchar_t* text);

#endif // !_COMMONFUN
