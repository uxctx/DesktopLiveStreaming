#pragma once
#include "AppContext.h"
#include "DXGIDesktopOutputSystem.h"
#include "BGRAFrameMemoryNode.h"
#include "H264_Encoder.h"

#define SharedBGRAFrameMemoryNodeCount 3

class DesktopImageSource
{
private:
	DXGIDesktopOutputSystem* d3dsys;

	int videoWidth;
	int videoHeight;

	ID3D11Texture2D *sharedTexture;
	ID3D11Texture2D *copyDataTexture;

	ID3D11Texture2D *bitBltTexture;
	ID3D11ShaderResourceView *bitBltTextureResource;

	HANDLE threadHandle[2];

	HANDLE encodeEvent;
	bool isRun;
	bool isload = false;

	BGRAFrameMemoryNode node[SharedBGRAFrameMemoryNodeCount];
	BGRAFrameMemoryNode * volatile worknode;

	H264_Encoder *encoder;
public:
	DesktopImageSource(class DXGIDesktopOutputSystem *d3dsys,  int maxBitrate);
	~DesktopImageSource();

	bool Init();
	bool Start();
	void Stop();

	void AcquireNextFrameData();
	void EncodedData();
};

