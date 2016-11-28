#pragma once
#include "AppContext.h"

class BGRAFrameMemoryNode
{
private:
	
	bool isload = false;
public:
	x264_picture_t pic;
	BGRAFrameMemoryNode() {};
	//BGRAFrameMemoryNode(int length) { data = app_malloc(length);isload = true; }
	~BGRAFrameMemoryNode() { 
		x264_picture_clean(&pic);
	}

	void init(int width, int height)
	{
		if (isload)
			return;
		x264_picture_alloc(&pic, X264_CSP_I420, width, height);
		isload = true;
	}

	

	BGRAFrameMemoryNode *next;
};
