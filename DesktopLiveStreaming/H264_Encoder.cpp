#include "stdafx.h"
#include "H264_Encoder.h"

#include "AppContext.h"

H264_Encoder::H264_Encoder(int width, int height, int maxBitrate)
{

	x264_param_default(&param);

	//int ret = x264_param_default_preset(&param, "veryfast", "zerolatency");
	int ret = x264_param_default_preset(&param, "veryfast", NULL);
	/*x264_param_apply_preset and x264_param_apply_tune*/

	param.i_width = width;
	param.i_height = height;

	param.i_csp = X264_CSP_I420;
	//param.i_csp = X264_CSP_BGRA;

	param.b_deterministic = false;

	param.i_fps_num = 25;
	param.i_fps_den = 1;

	param.i_timebase_num = 1;
	param.i_timebase_den = 1000;

	param.rc.i_vbv_max_bitrate = maxBitrate;
	param.rc.i_vbv_buffer_size = maxBitrate;
	param.rc.i_bitrate = maxBitrate;

	param.rc.i_rc_method = X264_RC_ABR;
	param.rc.f_rf_constant = 0.0f;

	param.b_vfr_input = 0;

	param.vui.b_fullrange = 0;
	param.vui.i_colorprim = 1;
	param.vui.i_transfer = 13;
	param.vui.i_colmatrix = 1;

	ret = x264_param_apply_profile(&param, "high");

	param.i_keyint_max = 30;

	param.i_threads = 1;
	this->x264 = x264_encoder_open(&param);
	if (this->x264 == NULL)
	{
		CrashErrorMessage("x264_encoder_open error~");
		exit(0);
	}
	//init sps pps

	x264_nal_t *nalOut;
	int nalNum;

	x264_encoder_headers(x264, &nalOut, &nalNum);

	char *sps_pps = this->sps_pps_data;
	char *sps_pps_ts = this->sps_pps_data_ts;
	for (int i = 0; i < nalNum; i++)
	{
		x264_nal_t &nal = nalOut[i];

		if (nal.i_type == NAL_SPS)
		{
			//sps
			static  char tempheader[] = { 0x17,0x0,0x0,0x0,0x0,0x1 };
			memcpy(sps_pps, tempheader, 6);
			sps_pps += 6;

			memcpy(sps_pps, nal.p_payload + 5, 3);
			sps_pps += 3;

			static  char tempheader2[] = { 0xff, 0xe1 };
			memcpy(sps_pps, tempheader2, 2);
			sps_pps += 2;

			WORD size = htons(nal.i_payload - 4);

			memcpy(sps_pps, &size, 2);
			sps_pps += 2;

			memcpy(sps_pps, nal.p_payload + 4, nal.i_payload - 4);
			sps_pps += (nal.i_payload - 4);

			sps_pps_ts = (char*)app_memcpy(sps_pps_ts, nal.p_payload, nal.i_payload);

			//pps

			x264_nal_t &pps = nalOut[i + 1]; //the PPS always comes after the SPS

			static  char ppss = 0x1;

			memcpy(sps_pps, &ppss, 1);
			sps_pps += 1;

			WORD ppssize = htons(pps.i_payload - 4);
			memcpy(sps_pps, &ppssize, 2);
			sps_pps += 2;

			memcpy(sps_pps, pps.p_payload + 4, pps.i_payload - 4);
			sps_pps += pps.i_payload - 4;

			sps_pps_ts = (char*)app_memcpy(sps_pps_ts, pps.p_payload, pps.i_payload);
			this->sps_pps_length = sps_pps - this->sps_pps_data;
			this->sps_pps_length_ts = sps_pps_ts - this->sps_pps_data_ts;
			break;
		}
	}
}

H264_Encoder::~H264_Encoder()
{
	x264_encoder_close(this->x264);
}

void H264_Encoder::SaveSei(char * data, int length)
{
	memcpy(this->sei_data, data, length);
	this->sei_length = length;
}

x264_t * H264_Encoder::x264encoder()
{
	return this->x264;
}

int H264_Encoder::get_sps_pps(char * buffer)
{
	memcpy(buffer, this->sps_pps_data, this->sps_pps_length);
	return this->sps_pps_length;
}

int H264_Encoder::get_sps_pps_ts(char * buffer)
{
	memcpy(buffer, this->sps_pps_data_ts, this->sps_pps_length_ts);
	return this->sps_pps_length_ts;
}

char* H264_Encoder::get_sei(char * buffer)
{
	memcpy(buffer, this->sei_data, this->sei_length);
	return buffer + this->sei_length;
}

