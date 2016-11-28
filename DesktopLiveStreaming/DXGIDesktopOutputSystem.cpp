#include "stdafx.h"
#include "DXGIDesktopOutputSystem.h"
#include "CommonFun.h"
#include "AppContext.h"

#define NUMVERTICES 6

DXGIDesktopOutputSystem::DXGIDesktopOutputSystem()
{
}

DXGIDesktopOutputSystem::~DXGIDesktopOutputSystem()
{
	this->desktopTexture2D->Release();
	RELEASE(m_DeskDupl);
	RELEASE(m_BlendState);
	RELEASE(d3d11PixelShader);
	RELEASE(d3d11InputLayout);
	RELEASE(d3d11VertexShader);
	RELEASE(d3d11DContext);
	RELEASE(d3d11Device);
}

bool DXGIDesktopOutputSystem::Init()
{

	if (!D3D11System::Init())
	{
		return false;
	}

	HRESULT hr = S_OK;
	//下面的代码就是win8以上平台才能运行

	// QI for Output 1
	IDXGIOutput1* DxgiOutput1 = nullptr;
	hr = DxgiOutput->QueryInterface(__uuidof(IDXGIOutput1), reinterpret_cast<void**>(&DxgiOutput1));
	DxgiOutput->Release();
	DxgiOutput = nullptr;
	if (FAILED(hr))
	{
		//CrashError(hr, L"QueryInterface IDXGIOutput1");
		DXG1NextFramecanWork = false;
		return false;
	}

	// Create desktop duplication
	hr = DxgiOutput1->DuplicateOutput(d3d11Device, &m_DeskDupl);
	DxgiOutput1->Release();
	DxgiOutput1 = nullptr;
	if (FAILED(hr))
	{
		DXG1NextFramecanWork = false;
		//CrashError(hr, L"IDXGIOutput1 DuplicateOutput");
		return false;
	}
	DXG1NextFramecanWork = true;
	return true;
}

AcquireFrameeEnum  DXGIDesktopOutputSystem::AcquireNextFrame(ID3D11Texture2D** desktop)
{
	DXGI_OUTDUPL_FRAME_INFO FrameInfo;

	HRESULT	hr = m_DeskDupl->AcquireNextFrame(15, &FrameInfo, &desktopResource);
	if (hr == DXGI_ERROR_WAIT_TIMEOUT)
	{
		//CrashError(hr, L"AcquireNextFrame WAIT_TIMEOUT");
		*desktop = this->desktopTexture2D;
		return AcquireFrameeEnum::Acquire_TIME_OUT;
	}
	if (FAILED(hr))
	{
		CrashError(hr, L"AcquireNextFrame ");
		return AcquireFrameeEnum::Acquire_ERROR;
	}
	hr = desktopResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&this->desktopTexture2D));
	if (FAILED(hr))
	{
		CrashError(hr, L" QueryInterface ID3D11Texture2D ");
		return AcquireFrameeEnum::Acquire_ERROR;
	}
	this->desktopResource->Release();
	this->desktopResource = nullptr;
	*desktop = this->desktopTexture2D;
	return AcquireFrameeEnum::Acquire_OK;
}

void DXGIDesktopOutputSystem::BitBltCopy(HDC & hDC, int width,int  height)
{
	HDC hCaptureDC = GetDC(NULL); 
	if (!BitBlt(hDC, 0, 0, width, height, hCaptureDC, 0, 0, SRCCOPY))
	{
		CrashErrorWIN32("BitBlt Error");
		exit(0);
	}
	ReleaseDC(NULL, hCaptureDC);

}

void DXGIDesktopOutputSystem::ReleaseFrame()
{
	if (this->desktopTexture2D != nullptr)
		this->desktopTexture2D->Release();//释放上一个
		//this->desktopTexture2D = nullptr;
	HRCHECK(this->m_DeskDupl->ReleaseFrame(), L"ReleaseFrame");
}

bool DXGIDesktopOutputSystem::GetTextureDC(ID3D11Texture2D * desktopTexture2D, HDC & hDC, IDXGISurface1** surface)
{
	HRESULT err;
	if (FAILED(err = desktopTexture2D->QueryInterface(__uuidof(IDXGISurface1), (void**)surface)))
	{
		CrashError(err, L">QueryInterface(__uuidof(IDXGISurface1) ");
		return false;
	}

	if (FAILED(err = (*surface)->GetDC(TRUE, &hDC)))
	{
		CrashErrorWIN32("D3D11Texture::GetDC(TRUE, &hDC))");
		//CrashError(err, L"D3D10Texture::GetDC(TRUE, &hDC))");
		(*surface)->Release();
		return false;
	}

	return true;
}

