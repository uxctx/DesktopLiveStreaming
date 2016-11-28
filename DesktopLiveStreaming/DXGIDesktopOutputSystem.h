#pragma once

#ifndef  _DXGIDesktopOutputSystem
#define _DXGIDesktopOutputSystem

#include "CommonFun.h"
#include "CommonTypes.h"
#include "D3D11System.h"

#include <d3d11.h>
#include <dxgi1_2.h>

enum AcquireFrameeEnum
{
	Acquire_OK,
	Acquire_TIME_OUT,
	Acquire_ERROR
};

class DXGIDesktopOutputSystem:public D3D11System
{
private:
	IDXGIOutputDuplication* m_DeskDupl = nullptr;
	
	ID3D11Texture2D* desktopTexture2D = nullptr;
	IDXGIResource* desktopResource = nullptr;

public:
	DXGIDesktopOutputSystem();
	~DXGIDesktopOutputSystem();

	bool Init();

	AcquireFrameeEnum AcquireNextFrame(ID3D11Texture2D** desktopTexture2D);

	bool DXG1NextFramecanWork;

	static void BitBltCopy(HDC & hDC, int width, int  height);

	void ReleaseFrame();

	static bool GetTextureDC(ID3D11Texture2D* desktopTexture2D, HDC &hDC, IDXGISurface1** surface);

};

#endif // ! _DXGIDesktopOutputSystem