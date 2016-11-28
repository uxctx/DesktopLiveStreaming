#pragma once
#include <d3d11.h>
#include <dxgi1_2.h>

#include "CommonFun.h"
#include "CommonTypes.h"

using namespace DirectX;

class WorkTexture
{
public:
	ID3D11Texture2D *texture2d;
	ID3D11ShaderResourceView *resource;
	ID3D11RenderTargetView *targetview;
	int width;
	int height;
	WorkTexture() {}
	~WorkTexture()
	{
		if (resource)
			resource->Release();
		if (targetview)
			targetview->Release();
		if (texture2d)
			texture2d->Release();
		targetview = nullptr;
		texture2d = nullptr;
		resource = nullptr;

	}
};

class D3D11System
{
protected:
	ID3D11VertexShader* d3d11VertexShader = nullptr;
	ID3D11PixelShader* d3d11PixelShader = nullptr;
	ID3D11InputLayout* d3d11InputLayout = nullptr;
	ID3D11SamplerState* d3d11SamplerLinear = nullptr;
	ID3D11BlendState* m_BlendState = nullptr;
	
	IDXGIOutput* DxgiOutput;

	DXGI_OUTPUT_DESC m_OutputDesc;
	ID3D11Device* d3d11Device = nullptr;
	ID3D11DeviceContext* d3d11DContext = nullptr;

public:
	D3D11System();
	~D3D11System();

	inline DXGI_OUTPUT_DESC* getOutputDesc() { return &m_OutputDesc; }
	inline ID3D11Device* getD3D11Device() { return d3d11Device; }
	inline ID3D11DeviceContext* getD3D11DContext()  { return d3d11DContext; }

	

	bool Init();

	//new 
	bool CreateSharedTexture2D(D3D11_SUBRESOURCE_DATA *pInitialData, D3D11_TEXTURE2D_DESC *texture_desc, WorkTexture* texture);
	bool DrawTexture(WorkTexture* target, WorkTexture* shader);
	bool DrawTexture(WorkTexture* target, WorkTexture* shader, float x, float y, float w, float h);
};

