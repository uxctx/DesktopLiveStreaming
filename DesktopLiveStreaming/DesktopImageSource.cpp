#include "stdafx.h"
#include <process.h> 

#include "DesktopImageSource.h"
#include "DebugExecutionTime.h"
#include "RingCacheBuffer.h"


#ifdef __cplusplus
extern "C" {
#endif

#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#ifdef __cplusplus
}
#endif

static LPBYTE GetBitmapData(HBITMAP hBmp, BITMAP &bmp)
{
	if (!hBmp)
		return NULL;

	if (GetObject(hBmp, sizeof(bmp), &bmp) != 0) {
		UINT bitmapDataSize = bmp.bmHeight*bmp.bmWidth*bmp.bmBitsPixel;
		bitmapDataSize >>= 3;

		LPBYTE lpBitmapData = (LPBYTE)malloc(bitmapDataSize);
		GetBitmapBits(hBmp, bitmapDataSize, lpBitmapData);

		return lpBitmapData;
	}

	return NULL;
}

static inline BYTE BitToAlpha(LPBYTE lp1BitTex, int pixel, bool bInvert)
{
	BYTE pixelByte = lp1BitTex[pixel / 8];
	BYTE pixelVal = pixelByte >> (7 - (pixel % 8)) & 1;

	if (bInvert)
		return pixelVal ? 0xFF : 0;
	else
		return pixelVal ? 0 : 0xFF;
}

static LPBYTE GetCursorData(HICON hIcon, ICONINFO &ii, UINT &width, UINT &height)
{
	BITMAP bmpColor, bmpMask;
	LPBYTE lpBitmapData = NULL, lpMaskData = NULL;

	if (lpBitmapData = GetBitmapData(ii.hbmColor, bmpColor)) {
		if (bmpColor.bmBitsPixel < 32) {
			free(lpBitmapData);
			return NULL;
		}

		if (lpMaskData = GetBitmapData(ii.hbmMask, bmpMask)) {
			int pixels = bmpColor.bmHeight*bmpColor.bmWidth;
			bool bHasAlpha = false;

			//god-awful horrible hack to detect 24bit cursor
			for (int i = 0; i < pixels; i++) {
				if (lpBitmapData[i * 4 + 3] != 0) {
					bHasAlpha = true;
					break;
				}
			}

			if (!bHasAlpha) {
				for (int i = 0; i < pixels; i++) {
					lpBitmapData[i * 4 + 3] = BitToAlpha(lpMaskData, i, false);
				}
			}

			free(lpMaskData);
		}

		width = bmpColor.bmWidth;
		height = bmpColor.bmHeight;
	}
	else if (lpMaskData = GetBitmapData(ii.hbmMask, bmpMask)) {
		bmpMask.bmHeight /= 2;

		int pixels = bmpMask.bmHeight*bmpMask.bmWidth;
		lpBitmapData = (LPBYTE)malloc(pixels * 4);
		ZeroMemory(lpBitmapData, pixels * 4);

		UINT bottom = bmpMask.bmWidthBytes*bmpMask.bmHeight;

		for (int i = 0; i < pixels; i++) {
			BYTE transparentVal = BitToAlpha(lpMaskData, i, false);
			BYTE colorVal = BitToAlpha(lpMaskData + bottom, i, true);

			if (!transparentVal)
				lpBitmapData[i * 4 + 3] = colorVal; //as an alternative to xoring, shows inverted as black
			else
				*(LPDWORD)(lpBitmapData + (i * 4)) = colorVal ? 0xFFFFFFFF : 0xFF000000;
		}

		free(lpMaskData);

		width = bmpMask.bmWidth;
		height = bmpMask.bmHeight;
	}

	return lpBitmapData;
};

UINT _stdcall AcquireNextFrameThread(LPVOID lParam)
{
	AppContext::context->desktopSource->AcquireNextFrameData();
	return 0;
};

UINT _stdcall DataEncodedThread(LPVOID lParam)
{
	AppContext::context->desktopSource->EncodedData();
	return 0;
};

DesktopImageSource::DesktopImageSource(DXGIDesktopOutputSystem * d3dsys, int maxBitrate)
{
	this->d3dsys = d3dsys;
	RECT* outputDesc = &d3dsys->getOutputDesc()->DesktopCoordinates;
	this->videoWidth = outputDesc->right - outputDesc->left;
	this->videoHeight = outputDesc->bottom - outputDesc->top;

	MyContext->SetVideoSize(this->videoWidth, this->videoHeight);

	encoder = new H264_Encoder(this->videoWidth, this->videoHeight, maxBitrate);
	MyContext->h264Encoder = encoder;

	this->isRun = false;

	size_t videoFrameSize = this->videoWidth*this->videoHeight * 4;
	worknode = &node[SharedBGRAFrameMemoryNodeCount - 1];
	BGRAFrameMemoryNode *temp;
	for (size_t i = 0; i < SharedBGRAFrameMemoryNodeCount; i++)
	{
		temp = &node[i];
		temp->init(this->videoWidth, this->videoHeight);
		worknode->next = temp;
		worknode = temp;
	}

};

DesktopImageSource::~DesktopImageSource()
{
	delete this->encoder;
};

bool DesktopImageSource::Init()
{
	if (this->isload)
		return true;
	this->encodeEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	if (this->encodeEvent == NULL)
	{
		CrashErrorWIN32("CreateEvent");
		return false;
	}
	D3D11_TEXTURE2D_DESC deskTexD;
	RtlZeroMemory(&deskTexD, sizeof(D3D11_TEXTURE2D_DESC));
	deskTexD.Width = this->videoWidth;
	deskTexD.Height = this->videoHeight;
	deskTexD.MipLevels = 1;
	deskTexD.ArraySize = 1;
	deskTexD.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	deskTexD.SampleDesc.Count = 1;
	deskTexD.Usage = D3D11_USAGE_DEFAULT;
	deskTexD.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	HRESULT	hr = this->d3dsys->getD3D11Device()->CreateTexture2D(&deskTexD, nullptr, &sharedTexture);
	if (FAILED(hr))
	{
		CloseHandle(this->encodeEvent);
		CrashError(hr, L"CreateTexture2D sharedTexture");
		return false;
	}

	RtlZeroMemory(&deskTexD, sizeof(D3D11_TEXTURE2D_DESC));

	deskTexD.Width = this->videoWidth;
	deskTexD.Height = this->videoHeight;
	deskTexD.MipLevels = 1;
	deskTexD.ArraySize = 1;
	deskTexD.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	deskTexD.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	deskTexD.SampleDesc.Count = 1;

	deskTexD.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;

	//deskTexD.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	deskTexD.MiscFlags = D3D11_RESOURCE_MISC_GDI_COMPATIBLE;

	hr = this->d3dsys->getD3D11Device()->CreateTexture2D(&deskTexD, nullptr, &bitBltTexture);
	if (FAILED(hr))
	{
		CloseHandle(this->encodeEvent);
		CrashError(hr, L"CreateTexture2D bitBltTexture");
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC resourceDesc;
	RtlZeroMemory(&resourceDesc, sizeof(resourceDesc));
	resourceDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	resourceDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	resourceDesc.Texture2D.MipLevels = 1;

	hr = this->d3dsys->getD3D11Device()->CreateShaderResourceView(bitBltTexture, &resourceDesc, &bitBltTextureResource);
	if (FAILED(hr))
	{
		CloseHandle(this->encodeEvent);
		CrashError(hr, L"CreateShaderResourceView(bitBltTexture");
		return false;
	}

	RtlZeroMemory(&deskTexD, sizeof(D3D11_TEXTURE2D_DESC));
	deskTexD.Width = this->videoWidth;
	deskTexD.Height = this->videoHeight;
	deskTexD.MipLevels = 1;
	deskTexD.ArraySize = 1;
	deskTexD.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	deskTexD.SampleDesc.Count = 1;

	deskTexD.Usage = D3D11_USAGE::D3D11_USAGE_STAGING;

	deskTexD.BindFlags = 0;
	deskTexD.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	deskTexD.MiscFlags = 0;

	hr = this->d3dsys->getD3D11Device()->CreateTexture2D(&deskTexD, nullptr, &copyDataTexture);
	if (FAILED(hr))
	{
		CloseHandle(this->encodeEvent);
		CrashError(hr, L"CreateTexture2D copyDataTexture");
		return false;
	}

	this->isload = true;
	return true;
}

bool DesktopImageSource::Start()
{
	this->isRun = true;
	threadHandle[0] = (HANDLE)_beginthreadex(NULL, 0, AcquireNextFrameThread, NULL, 0, NULL);
	threadHandle[1] = (HANDLE)_beginthreadex(NULL, 0, DataEncodedThread, NULL, 0, NULL);
	return true;
}

void DesktopImageSource::Stop()
{
	this->isRun = false;

	SetEvent(this->encodeEvent);

	WaitForMultipleObjects(2, threadHandle, TRUE, INFINITE);
	CloseHandle(threadHandle[0]);
	CloseHandle(threadHandle[1]);
}

void DesktopImageSource::AcquireNextFrameData()
{
	//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

	ID3D11DeviceContext* d3dDeviceContext = this->d3dsys->getD3D11DContext();
	ID3D11Device* d3dDevice = this->d3dsys->getD3D11Device();

	D3D11_SHADER_RESOURCE_VIEW_DESC ShaderDesc;
	ShaderDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	ShaderDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	ShaderDesc.Texture2D.MostDetailedMip = 0;
	ShaderDesc.Texture2D.MipLevels = 1;

	HRESULT hr;

	WorkTexture	nextFrame;
	WorkTexture	target;

	WorkTexture	bitBltWork;

	ID3D11ShaderResourceView* ShaderResource = nullptr;
	CURSORINFO ci;
	UINT cursorWidth = 0, cursorHeight = 0;
	POINT    cursorPos;
	HCURSOR  hCurrentCursor = nullptr;
	WorkTexture  cursorTexture;
	int     xHotspot, yHotspot;
	HICON hIcon;

	target.height = this->videoHeight;
	target.width = this->videoWidth;
	target.texture2d = sharedTexture;

	bitBltWork.height = this->videoHeight;
	bitBltWork.width = this->videoWidth;

	bitBltWork.texture2d = bitBltTexture;
	bitBltWork.resource = bitBltTextureResource;

	HDC hDC;
	IDXGISurface1 *surface;

	D3D11_MAPPED_SUBRESOURCE mapOutResource;
	size_t videoFrameSize = this->videoWidth*this->videoHeight * 4;
	BGRAFrameMemoryNode *next = nullptr;

	AcquireFrameeEnum acquireRelust;

	SwsContext *img_convert_ctx = sws_getContext(
		this->videoWidth, this->videoHeight, AV_PIX_FMT_BGRA,//源
		this->videoWidth, this->videoHeight, AV_PIX_FMT_YUV420P,//目标
		SWS_FAST_BILINEAR, NULL, NULL, NULL);

	AVFrame *frame_in = av_frame_alloc();
	uint8_t *frame_buffer_in = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_BGRA,
		this->videoWidth, this->videoHeight, 1));
	av_image_fill_arrays(frame_in->data, frame_in->linesize, frame_buffer_in,
		AV_PIX_FMT_BGRA, this->videoWidth, this->videoHeight, 1);

	AVFrame *frame_out = av_frame_alloc();
	uint8_t *frame_buffer_out = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P,
		this->videoWidth, this->videoHeight, 1));
	av_image_fill_arrays(frame_out->data, frame_out->linesize, frame_buffer_out,
		AV_PIX_FMT_YUV420P, this->videoWidth, this->videoHeight, 1);

	int half_width = (this->videoWidth + 1) / 2;
	int half_height = (this->videoHeight + 1) / 2;

	BGRAFrameMemoryNode volatile * newyuv = worknode;

	SetEvent(this->encodeEvent);
	while (this->isRun)
	{
		if (WaitForSingleObject(this->encodeEvent, INFINITE) != WAIT_OBJECT_0)
		{
			CrashError(hr, L"WaitForSingleObject encodeEvent");
			break;
		}

		//this->d3dsys->DXG1NextFramecanWork = false;<-this test
		if (this->d3dsys->DXG1NextFramecanWork)
		{

			acquireRelust = this->d3dsys->AcquireNextFrame(&nextFrame.texture2d);

			//msdn解释:The format of the desktop image is always DXGI_FORMAT_B8G8R8A8_UNORM no matter what the current display mode is. 

			switch (acquireRelust)
			{
			case Acquire_OK:
				//d3dsys->ReleaseFrame();
				break;
			case Acquire_TIME_OUT:

				goto DrawTexture;
				//continue;//while 
				break;
			case Acquire_ERROR:
				CrashError(hr, L"AcquireNextFrame");
				break;
			default:
				break;
			}

			hr = d3dDevice->CreateShaderResourceView(nextFrame.texture2d, &ShaderDesc, &ShaderResource);
			if (FAILED(hr)) {
				CrashError(hr, L"CreateShaderResourceView");
				break;
			}

			nextFrame.resource = ShaderResource;

			d3dsys->DrawTexture(&target, &nextFrame);

			nextFrame.resource->Release();
			nextFrame.resource = nullptr;

			if (acquireRelust == Acquire_OK)
				d3dsys->ReleaseFrame();

			ZeroMemory(&ci, sizeof(ci));
			ci.cbSize = sizeof(ci);
			if (GetCursorInfo(&ci))
			{
				memcpy(&cursorPos, &ci.ptScreenPos, sizeof(cursorPos));
				if (ci.flags & CURSOR_SHOWING)//
				{
					if (ci.hCursor != hCurrentCursor)
					{
						//开始重新获取鼠标图标信息！
						hIcon = CopyIcon(ci.hCursor);
						hCurrentCursor = ci.hCursor;

						if (hIcon)
						{
							ICONINFO ii;
							if (GetIconInfo(hIcon, &ii))
							{
								xHotspot = int(ii.xHotspot);//
								yHotspot = int(ii.yHotspot);

								LPBYTE lpData = GetCursorData(hIcon, ii, cursorWidth, cursorHeight);//
								if (lpData && cursorWidth && cursorHeight)
								{
									D3D11_TEXTURE2D_DESC td;
									ZeroMemory(&td, sizeof(td));
									td.Width = cursorWidth;
									td.Height = cursorHeight;
									td.MipLevels = 1;
									td.ArraySize = 1;
									td.Format = DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM;
									td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
									td.SampleDesc.Count = 1;
									td.Usage = D3D11_USAGE_DEFAULT;
									td.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

									D3D11_SUBRESOURCE_DATA srd;
									srd.pSysMem = lpData;
									srd.SysMemPitch = cursorHeight * 4;//rgba->4
									srd.SysMemSlicePitch = 0;

									d3dsys->CreateSharedTexture2D(&srd, &td, &cursorTexture);

									free(lpData);
								}

								DeleteObject(ii.hbmColor);
								DeleteObject(ii.hbmMask);
							}

							DestroyIcon(hIcon);
						}
					}
				}
			}
			else
			{
				nextFrame.resource->Release();
				nextFrame.resource = nullptr;
				d3dsys->ReleaseFrame();

				CrashErrorMessage("GetCursorInfo");
				return;
			}

DrawTexture:
			d3dsys->DrawTexture(&target, &cursorTexture, ci.ptScreenPos.x - xHotspot, ci.ptScreenPos.y - yHotspot, cursorWidth, cursorHeight);

			d3dDeviceContext->CopyResource(copyDataTexture, target.texture2d);
		}
		else
		{
			DXGIDesktopOutputSystem::GetTextureDC(bitBltTexture, hDC, &surface);
			this->d3dsys->BitBltCopy(hDC, this->videoWidth, this->videoHeight);
			ZeroMemory(&ci, sizeof(ci));
			ci.cbSize = sizeof(ci);
			if (GetCursorInfo(&ci))
			{
				memcpy(&cursorPos, &ci.ptScreenPos, sizeof(cursorPos));
				if (ci.flags & CURSOR_SHOWING)//
				{

					hIcon = CopyIcon(ci.hCursor);
					if (hIcon)
					{
						ICONINFO ii;
						if (GetIconInfo(hIcon, &ii))
						{
							POINT capturePos = { 0,0 };

							int x = ci.ptScreenPos.x - int(ii.xHotspot) - capturePos.x;
							int y = ci.ptScreenPos.y - int(ii.yHotspot) - capturePos.y;

							DrawIcon(hDC, x, y, hIcon);

							DeleteObject(ii.hbmColor);
							DeleteObject(ii.hbmMask);
						}

						DestroyIcon(hIcon);
					}

				}
			}
			surface->ReleaseDC(NULL);
			surface->Release();
			d3dDeviceContext->CopyResource(copyDataTexture, bitBltTexture);

		}

		next = newyuv->next;
		hr = d3dDeviceContext->Map(copyDataTexture, 0, D3D11_MAP::D3D11_MAP_READ, 0, &mapOutResource);
		memcpy(frame_in->data[0], mapOutResource.pData, videoFrameSize);
		d3dDeviceContext->Unmap(copyDataTexture, 0);

		int got_picture = sws_scale(img_convert_ctx,
			frame_in->data, frame_in->linesize, 0, this->videoHeight
			, frame_out->data, frame_out->linesize);

		memcpy(next->pic.img.plane[0], frame_out->data[0], this->videoWidth*this->videoHeight);
		memcpy(next->pic.img.plane[1], frame_out->data[1], half_width*half_height);
		memcpy(next->pic.img.plane[2], frame_out->data[2], half_width*half_height);

		//libyuv::BGRAToI420(iframeBRGAdata,
		//	this->videoWidth * 4,
		//	next->pic.img.plane[0], this->videoWidth,
		//	next->pic.img.plane[1], half_width,
		//	next->pic.img.plane[2], half_width,
		//	this->videoWidth, this->videoHeight);

		newyuv = next;
	}

	sws_freeContext(img_convert_ctx);

	av_frame_free(&frame_out);
	av_free(frame_buffer_out);

	av_frame_free(&frame_in);
	av_free(frame_buffer_in);
};

void DesktopImageSource::EncodedData()
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

	x264_picture_t *pic_in = nullptr;
	size_t videoFrameSize = this->videoWidth*this->videoHeight * 4;
	x264_t* x264 = this->encoder->x264encoder();

	x264_picture_t pic_out;
	x264_picture_init(&pic_out);

	x264_nal_t* pNals = NULL;
	int iNal = 0;

	QWORD streamTimeStart = GetQPCTimeNS();
	QWORD frameTimeNS = 1000000000 / FPS;
	QWORD sleepTargetTime = streamTimeStart;

	//debug
	bool sleepch = false;

	//"NAL_UNKNOWN",
	//"NAL_SLICE" ,
	//"NAL_SLICE_DPA" ,
	//"NAL_SLICE_DPB" ,
	//"NAL_SLICE_DPC" ,
	//"NAL_SLICE_IDR" ,
	//"NAL_SEI" ,
	//"NAL_SPS" ,
	//"NAL_PPS",
	//"NAL_AUD" ,
	//"NAL_FILLER"

	int ret = 0;
	RingCacheBuffer* cache = MyContext->rCache;
	int frameShift = 0;
	int timeOffset;
	while (this->isRun)
	{
		sleepch = SleepToNS(sleepTargetTime += frameTimeNS);

		SetEvent(this->encodeEvent);

		if (!this->worknode) {
			Yield();
			continue;
		}

		pic_in = &this->worknode->pic;
		ret = x264_encoder_encode(x264, &pNals, &iNal, pic_in, &pic_out);

		timeOffset = int(pic_out.i_pts - pic_out.i_dts);
		timeOffset += frameShift;
		DWORD out_pts = (DWORD)pic_out.i_pts;
		if (iNal && timeOffset < 0) {

			frameShift -= timeOffset;
			timeOffset = 0;
		}

		QWORD now_ms = GetQPCTimeMS();
		for (int j = 0; j < iNal; ++j) {

			//DbgPrintA(" %s : %d  ", nal_names[pNals[j].i_type], pNals[j].i_payload);

			x264_nal_t &nal = pNals[j];

			if (nal.i_type == NAL_SLICE_IDR) {

				cache->overlay_video(true, (char*)nal.p_payload, nal.i_payload, now_ms, timeOffset);
				continue;
			}
			else if (nal.i_type == NAL_SLICE) {

				cache->overlay_video(false, (char*)nal.p_payload, nal.i_payload, now_ms, timeOffset);
				continue;
			}
			else if (nal.i_type == NAL_FILLER) {

				//DbgPrintA("\n");
			}
			//NAL_SEI
		}
		//DbgPrintA("\n");
	}
};

