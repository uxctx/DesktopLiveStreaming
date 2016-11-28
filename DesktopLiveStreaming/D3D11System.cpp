#include "stdafx.h"
#include "D3D11System.h"

D3D11System::D3D11System()
{

}

D3D11System::~D3D11System()
{
	RELEASE(DxgiOutput);

	RELEASE(m_BlendState);
	RELEASE(d3d11PixelShader);
	RELEASE(d3d11InputLayout);
	RELEASE(d3d11VertexShader);
	RELEASE(d3d11DContext);
	RELEASE(d3d11Device);
}

bool D3D11System::Init()
{

	HRESULT hr = S_OK;

	// Driver types supported 
	D3D_DRIVER_TYPE DriverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,//
		D3D_DRIVER_TYPE_WARP,//软件光栅
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT NumDriverTypes = ARRAYSIZE(DriverTypes);

	// Feature levels supported
	D3D_FEATURE_LEVEL FeatureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_1
	};
	UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);

	D3D_FEATURE_LEVEL FeatureLevel;

	// Create device
	for (UINT DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; ++DriverTypeIndex)
	{
		hr = D3D11CreateDevice(nullptr, DriverTypes[DriverTypeIndex], nullptr, 0, FeatureLevels, NumFeatureLevels,
			D3D11_SDK_VERSION, &d3d11Device, &FeatureLevel, &d3d11DContext);
		if (SUCCEEDED(hr))
		{
			break;
		}
	}
	HRCHECK(hr, L"D3D11CreateDevice");

	// VERTEX shader
	UINT Size = ARRAYSIZE(g_VS);
	HRCHECKTOEND(d3d11Device->CreateVertexShader(g_VS, Size, nullptr, &d3d11VertexShader), L"CreateVertexShader");

	// Input layout
	D3D11_INPUT_ELEMENT_DESC Layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	UINT NumElements = ARRAYSIZE(Layout);
	HRCHECKTOEND(d3d11Device->CreateInputLayout(Layout, NumElements, g_VS, Size, &d3d11InputLayout), L"CreateInputLayout ");
	d3d11DContext->IASetInputLayout(d3d11InputLayout);

	// Pixel shader
	Size = ARRAYSIZE(g_PS);
	HRCHECKTOEND(d3d11Device->CreatePixelShader(g_PS, Size, nullptr, &d3d11PixelShader), L"CreatePixelShader");

	// Set up sampler
	D3D11_SAMPLER_DESC SampDesc;
	RtlZeroMemory(&SampDesc, sizeof(SampDesc));
	SampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	SampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	SampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	SampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	SampDesc.MinLOD = 0;
	SampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	HRCHECKTOEND(d3d11Device->CreateSamplerState(&SampDesc, &d3d11SamplerLinear), L"CreateSamplerState");

	D3D11_BLEND_DESC BlendStateDesc;
	BlendStateDesc.AlphaToCoverageEnable = FALSE;
	BlendStateDesc.IndependentBlendEnable = FALSE;
	BlendStateDesc.RenderTarget[0].BlendEnable = TRUE;
	BlendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	BlendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	BlendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	BlendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	BlendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	BlendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	BlendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	HRCHECKTOEND(d3d11Device->CreateBlendState(&BlendStateDesc, &m_BlendState), L"CreateBlendState");

	// Get DXGI device
	IDXGIDevice* DxgiDevice = nullptr;
	HRCHECKTOEND(d3d11Device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&DxgiDevice)), L"QueryInterface IDXGIDevice");

	// Get DXGI adapter
	IDXGIAdapter* DxgiAdapter = nullptr;
	hr = DxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&DxgiAdapter));
	DxgiDevice->Release();
	DxgiDevice = nullptr;
	if (FAILED(hr))
	{
		CrashError(hr, L"IDXGIDevice GetParent");
		return false;
	}

	// Get output
	
	hr = DxgiAdapter->EnumOutputs(0, &DxgiOutput);//默认0
	DxgiAdapter->Release();
	DxgiAdapter = nullptr;
	if (FAILED(hr))
	{
		CrashError(hr, L"IDXGIAdapter EnumOutputs");
		return false;
	}

	DxgiOutput->GetDesc(&m_OutputDesc);

	return true;
error:
	RELEASE(m_BlendState);
	RELEASE(d3d11PixelShader);
	RELEASE(d3d11InputLayout);
	RELEASE(d3d11VertexShader);
	RELEASE(d3d11DContext);
	RELEASE(d3d11Device);

	return false;
}

bool D3D11System::CreateSharedTexture2D(D3D11_SUBRESOURCE_DATA *pInitialData, D3D11_TEXTURE2D_DESC *texture_desc, WorkTexture* texture)
{
	HRESULT hr = d3d11Device->CreateTexture2D(texture_desc, pInitialData, &texture->texture2d);
	if (FAILED(hr))
	{
		CrashError(hr, L"CreateTexture2D");
		return false;
	}
	D3D11_SHADER_RESOURCE_VIEW_DESC resourceDesc;
	ZeroMemory(&resourceDesc, sizeof(resourceDesc));
	resourceDesc.Format = texture_desc->Format;
	resourceDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	resourceDesc.Texture2D.MipLevels = 1;

	d3d11Device->CreateShaderResourceView(texture->texture2d, &resourceDesc, &texture->resource);
	return true;
}

bool D3D11System::DrawTexture(WorkTexture* target, WorkTexture* shader)
{
	HRESULT hr;
	if (!target->targetview)
	{
		//ID3D11RenderTargetView
		hr = d3d11Device->CreateRenderTargetView(target->texture2d, nullptr, &target->targetview);
		if (FAILED(hr))
		{
			CrashError(hr, L"CreateRenderTargetView");
			return false;
		}
	}

	VERTEX Vertices[NUMVERTICES] =
	{
		{ XMFLOAT3(-1.0f, -1.0f, 0), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 0), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 0), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 0), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 0), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 0), XMFLOAT2(1.0f, 0.0f) },
	};

	D3D11_BUFFER_DESC BDesc;
	ZeroMemory(&BDesc, sizeof(D3D11_BUFFER_DESC));
	BDesc.Usage = D3D11_USAGE_DEFAULT;
	BDesc.ByteWidth = sizeof(VERTEX) * NUMVERTICES;
	BDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	BDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(D3D11_SUBRESOURCE_DATA));
	InitData.pSysMem = Vertices;

	ID3D11Buffer* VertexBufferMouse = nullptr;
	
	hr = d3d11Device->CreateBuffer(&BDesc, &InitData, &VertexBufferMouse);
	if (FAILED(hr))
	{
		CrashError(hr, L"CreateBuffer");
		return false;
	}

	// Set resources
	FLOAT BlendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
	UINT Stride = sizeof(VERTEX);
	UINT Offset = 0;
	d3d11DContext->IASetVertexBuffers(0, 1, &VertexBufferMouse, &Stride, &Offset);
	d3d11DContext->OMSetBlendState(nullptr, BlendFactor, 0xFFFFFFFF);
	d3d11DContext->OMSetRenderTargets(1, &target->targetview, nullptr);
	d3d11DContext->VSSetShader(d3d11VertexShader, nullptr, 0);
	d3d11DContext->PSSetShader(d3d11PixelShader, nullptr, 0);
	d3d11DContext->PSSetShaderResources(0, 1, &shader->resource);
	d3d11DContext->PSSetSamplers(0, 1, &d3d11SamplerLinear);
	//d3d11DContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3D11_VIEWPORT VP;
	VP.Width = static_cast<FLOAT>(target->width);
	VP.Height = static_cast<FLOAT>(target->height);
	VP.MinDepth = 0.0f;
	VP.MaxDepth = 1.0f;
	VP.TopLeftX = 0.0f;
	VP.TopLeftY = 0.0f;
	d3d11DContext->RSSetViewports(1, &VP);//	绑定一个视口数组
										  // Draw
	d3d11DContext->Draw(NUMVERTICES, 0);

	// Clean
	if (VertexBufferMouse)
	{
		VertexBufferMouse->Release();
		VertexBufferMouse = nullptr;
	}

	return DUPL_RETURN_SUCCESS;
}

bool D3D11System::DrawTexture(WorkTexture* target, WorkTexture* shader, float x, float y, float w, float h)
{
	HRESULT hr;
	if (!target->targetview)
	{
		//ID3D11RenderTargetView
		hr = d3d11Device->CreateRenderTargetView(target->texture2d, nullptr, &target->targetview);
		if (FAILED(hr))
		{
			CrashError(hr, L"CreateRenderTargetView");
			return false;
		}
	}

	VERTEX Vertices[NUMVERTICES] =
	{
		{ XMFLOAT3(-1.0f, -1.0f, 0), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 0), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 0), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 0), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 0), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 0), XMFLOAT2(1.0f, 0.0f) },
	};

	INT CenterY = (target->height / 2);
	INT CenterX = (target->width / 2);

	INT PtrWidth = w;
	INT PtrHeight = h;
	INT PtrLeft = x;
	INT PtrTop = y;

	// Buffer used if necessary (in case of monochrome or masked pointer)
	BYTE* InitBuffer = nullptr;

	//char msg[1000];
	//sprintf_s(msg, 1000, "PtrLeft:%d,PtrTop:%d,PtrWidth:%d\n", PtrLeft, PtrTop, PtrWidth);
	//OutputDebugStringA(msg);
	// VERTEX creation
	Vertices[0].Pos.x = (PtrLeft - CenterX) / (FLOAT)CenterX;
	Vertices[0].Pos.y = -1 * ((PtrTop + PtrHeight) - CenterY) / (FLOAT)CenterY;
	Vertices[1].Pos.x = (PtrLeft - CenterX) / (FLOAT)CenterX;
	Vertices[1].Pos.y = -1 * (PtrTop - CenterY) / (FLOAT)CenterY;
	Vertices[2].Pos.x = ((PtrLeft + PtrWidth) - CenterX) / (FLOAT)CenterX;
	Vertices[2].Pos.y = -1 * ((PtrTop + PtrHeight) - CenterY) / (FLOAT)CenterY;
	Vertices[3].Pos.x = Vertices[2].Pos.x;
	Vertices[3].Pos.y = Vertices[2].Pos.y;
	Vertices[4].Pos.x = Vertices[1].Pos.x;
	Vertices[4].Pos.y = Vertices[1].Pos.y;
	Vertices[5].Pos.x = ((PtrLeft + PtrWidth) - CenterX) / (FLOAT)CenterX;
	Vertices[5].Pos.y = -1 * (PtrTop - CenterY) / (FLOAT)CenterY;

	//sprintf_s(msg,100,"%f,CenterY:%d\n", Vertices[2].Pos.y, CenterY);
	//OutputDebugStringA(msg);

	D3D11_BUFFER_DESC BDesc;
	ZeroMemory(&BDesc, sizeof(D3D11_BUFFER_DESC));
	BDesc.Usage = D3D11_USAGE_DEFAULT;
	BDesc.ByteWidth = sizeof(VERTEX) * NUMVERTICES;
	BDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	BDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(D3D11_SUBRESOURCE_DATA));
	InitData.pSysMem = Vertices;

	ID3D11Buffer* VertexBufferMouse = nullptr;
	// Create vertex buffer
	hr = d3d11Device->CreateBuffer(&BDesc, &InitData, &VertexBufferMouse);
	if (FAILED(hr))
	{
		CrashError(hr, L"CreateBuffer");
		return false;
	}

	// Set resources
	FLOAT BlendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
	UINT Stride = sizeof(VERTEX);
	UINT Offset = 0;
	d3d11DContext->IASetVertexBuffers(0, 1, &VertexBufferMouse, &Stride, &Offset);
	d3d11DContext->OMSetBlendState(m_BlendState, BlendFactor, 0xFFFFFFFF);
	d3d11DContext->OMSetRenderTargets(1, &target->targetview, nullptr);
	d3d11DContext->VSSetShader(d3d11VertexShader, nullptr, 0);
	d3d11DContext->PSSetShader(d3d11PixelShader, nullptr, 0);
	d3d11DContext->PSSetShaderResources(0, 1, &shader->resource);
	d3d11DContext->PSSetSamplers(0, 1, &d3d11SamplerLinear);
	//d3d11DContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//D3D11_VIEWPORT VP;
	//VP.Width = static_cast<FLOAT>(1920);
	//VP.Height = static_cast<FLOAT>(1080);
	//VP.MinDepth = 0.0f;
	//VP.MaxDepth = 1.0f;
	//VP.TopLeftX = 0.0f;
	//VP.TopLeftY = 0.0f;
	//d3d11DContext->RSSetViewports(1, &VP);//	绑定一个视口数组
	// Draw
	d3d11DContext->Draw(NUMVERTICES, 0);

	// Clean
	if (VertexBufferMouse)
	{
		VertexBufferMouse->Release();
		VertexBufferMouse = nullptr;
	}

	return DUPL_RETURN_SUCCESS;
}