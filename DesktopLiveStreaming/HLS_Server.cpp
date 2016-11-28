#include "stdafx.h"
#include "HLS_Server.h"
#include "H264_Encoder.h"
#include "AudioSoureAACStream.h"
#include "RingCacheBuffer.h"

#include <list>

#include "shlwapi.h"
#pragma comment(lib,"shlwapi.lib")

extern "C"
{
#include "mpegts.h"
};

UINT _stdcall TsFactoryWork(LPVOID lParam)
{
	((HLS_Server*)lParam)->work();
	return 0;
}


__forceinline void flush_audio(mpegts_frame_t &frame, mpegts_file_t &tsfile,
	uint32_t &audio_counter, mpegts_buf_t& aacout, u_char * aac_cache_buffer)
{
	if (aacout.last == aacout.start) {
		return;
	}
	frame.dts = frame.pts;
	//frame.pts = frame.dts;
	frame.cc = audio_counter;
	frame.pid = 0x101;
	frame.sid = 0xc0;
	frame.key = 0;
	mpegts_write_frame(&tsfile, &frame, &aacout);
	audio_counter = frame.cc;

	aacout.start = aac_cache_buffer;
	aacout.end = aac_cache_buffer + aac_frame_size;
	aacout.pos = aacout.start;
	aacout.last = aacout.pos;
}

__forceinline size_t update_m3u8_playlist(char *cache, char* data, std::list<ts_file> &tslist, app_atomic_lock_t *lock)
{
	/*#EXTM3U
	  #EXT-X-VERSION:3
	  #EXT-X-TARGETDURATION:10
	  #EXT-X-MEDIA-SEQUENCE:1479314988*/
	char *dst = cache;
	int len;
	std::list<ts_file>::iterator it = tslist.begin();
	int skip = tslist.size()- m3u8_ts_list_count-1;
	if(skip>0)
	for (size_t i = 0; i < skip; i++){
		it++;
	}

	dst += sprintf(dst,
		"#EXTM3U\n"
		"#EXT-X-VERSION:3\n"
		"#EXT-X-TARGETDURATION:%d\n"
		"#EXT-X-MEDIA-SEQUENCE:%d\n"
		, m3u8_ts_duration, it->sequence);

	for (size_t i = 0; i < m3u8_ts_list_count; i++) {
		if (it == tslist.end())
			break;
		if (it->sequence - 0.0 < 1e-6)
			break;
		dst += sprintf(dst,
			"#EXTINF:%.3f,\n"
			"%s\n"
			, it->extinf, it->url);
		it++;
	}
	len = dst - cache;

	app_atomic_lock(lock, m3u8_list_lock);
	memcpy(data, cache, len);
	app_atomic_unlock(lock, m3u8_list_lock);
	return len;
}

void HLS_Server::work()
{

	RCBItem* work_node = nullptr;
	QWORD firsttime = 0;
	char spspps[300];
	char aac_header[8];
	m3u8_data_lock.lock = 0;

	if (!PathIsDirectory(hls_cache_path)) {
		CreateDirectory(hls_cache_path, NULL);
	}

	rcb = MyContext->rCache;

	h264 = MyContext->h264Encoder;

	int spsppslen = h264->get_sps_pps_ts(spspps);

	AudioSoureAACStream *aacs = MyContext->accSource;
	uint32_t   objtype, srindex, chconf;

	int aac_headerlen = aacs->GetHeander(aac_header);

	u_char b0 = aac_header[2], b1 = aac_header[3];
	objtype = b0 >> 3;
	if (objtype > 4) {
		objtype = 2;
	}
	srindex = ((b0 << 1) & 0x0f) | ((b1 & 0x80) >> 7);
	chconf = (b1 >> 3) & 0x0f;

	while ((work_node = this->rcb->lastKeyFrame) == NULL) {
		WaitForSingleObject(this->rcb->new_event, INFINITE);
	}

	mpegts_frame_t frame;
	mpegts_file_t tsfile;
	uint32_t video_counter = 0, audio_counter = 0;

	u_char * buffer = (u_char*)app_malloc(h264_frame_size);

	u_char * aac_cache_buffer = (u_char*)app_malloc(aac_frame_size);

	mpegts_buf_t   out, aacout;

	aacout.start = aac_cache_buffer;
	aacout.end = aac_cache_buffer + aac_frame_size;
	aacout.pos = aacout.start;
	aacout.last = aacout.pos;

	size_t ts_file_num = 1;
	std::list<ts_file> tslist;

	ts_file temp;

	int splen = sprintf(temp.url, "/hls/%d.ts", ts_file_num);
	//sprintf(temp.filename, ".%s", temp.url);
	temp.filename[0] = '.';
	temp.filename[splen + 1] = '\0';
	memcpy(temp.filename + 1, temp.url, splen);
	temp.sequence = ts_file_num;

	tslist.push_back(temp);
	mpegts_open_file(&tsfile, temp.filename);
	firsttime = work_node->timems;

	ts_file* writefile = &*(tslist.begin());

	while (this->run)
	{
		if (work_node->tagType == TagType::VideoTag)
		{
			if (work_node->isKeyFrame) {
				QWORD extinf_long = (float)(work_node->timems - firsttime);
				if (extinf_long >
					((m3u8_ts_duration - 1) * 1000)) {

					writefile->extinf = (float)extinf_long / 1000;
					flush_audio(frame, tsfile, audio_counter, aacout, aac_cache_buffer);
					mpegts_close_file(&tsfile);

					this->m3u8_data_len =
						update_m3u8_playlist(this->m3u8_data_copy, this->m3u8_data, tslist, &m3u8_data_lock);

					ts_file_num++;

					ts_file temp;
					splen = sprintf(temp.url, "/hls/%d.ts", ts_file_num);
					//sprintf(temp.filename, ".%s", temp.url);
					temp.filename[0] = '.';
					temp.filename[splen + 1] = '\0';
					memcpy(temp.filename + 1, temp.url, splen);
					temp.sequence = ts_file_num;

					if (tslist.size() > (
						m3u8_ts_list_count*3)) {
						ts_file *ts_remove = &tslist.front();
						DeleteFileA(ts_remove->filename);
						tslist.pop_front();
					}

					tslist.push_back(temp);
					writefile = &*(--tslist.end());

					mpegts_open_file(&tsfile, temp.filename);
					firsttime = work_node->timems;

				}
			}

			memset(&frame, sizeof(frame), 0);

			frame.cc = video_counter;
			frame.dts = (uint64_t)(work_node->timems) * 90;
			frame.pts = frame.dts + work_node->compositionTime * 90;
			frame.pid = 0x100;
			frame.sid = 0xe0;
			frame.key = work_node->isKeyFrame;

			out.start = buffer;
			out.end = buffer + h264_frame_size;
			out.pos = out.start;
			out.last = out.pos;

			out.last = (u_char*)app_memcpy(out.last, aud_nal, sizeof(aud_nal));

			if (work_node->isKeyFrame) {
				out.last = (u_char*)app_memcpy(out.last, spspps, spsppslen);
			}

			out.last =
				(u_char*)app_memcpy(out.last,
				(work_node->tag_body_isbig ? work_node->tag_body_big : work_node->tag_body_default),
					work_node->tag_body_length);

			mpegts_write_frame(&tsfile, &frame, &out);
			video_counter = frame.cc;

		}
		else if (work_node->tagType == TagType::AudioTag)
		{
			size_t 	size = work_node->tag_body_length - 2 + 7;
			if (aacout.last + size > aacout.end) {
				flush_audio(frame, tsfile, audio_counter, aacout, aac_cache_buffer);
			}
			if (aacout.last == aacout.start) {
				frame.pts = (uint64_t)(work_node->timems) * 90;
			}

			u_char *p = aacout.last;

			aacout.last += 5;
			aacout.last = (u_char*)app_memcpy(aacout.last,
				(work_node->tag_body_isbig ? work_node->tag_body_big : work_node->tag_body_default),
				work_node->tag_body_length);

			p[0] = 0xff;
			p[1] = 0xf1;
			p[2] = (u_char)(((objtype - 1) << 6) | (srindex << 2) |
				((chconf & 0x04) >> 2));
			p[3] = (u_char)(((chconf & 0x03) << 6) | ((size >> 11) & 0x03));
			p[4] = (u_char)(size >> 3);
			p[5] = (u_char)((size << 5) | 0x1f);
			p[6] = 0xfc;

		}
		while (work_node->index ==
			rcb->ReadLastWrite->index)
		{
			WaitForSingleObject(this->rcb->new_event, INFINITE);
		}
		work_node = work_node->next;
	}

}

HLS_Server::HLS_Server()
{
}

HLS_Server::~HLS_Server()
{
}

void HLS_Server::start()
{
	this->run = true;
	thread[0] = (HANDLE)_beginthreadex(NULL, 0, TsFactoryWork, this, 0, NULL);
}

void HLS_Server::stop()
{
}

int HLS_Server::get_m3u8_data(char * dst)
{
	if (m3u8_data_lock.lock != m3u8_list_lock) {
		memcpy(dst, this->m3u8_data, this->m3u8_data_len);
		return this->m3u8_data_len;
	}
	app_atomic_lock(&m3u8_data_lock, m3u8_list_lock_read);
	memcpy(dst, this->m3u8_data, this->m3u8_data_len);
	app_atomic_unlock(&m3u8_data_lock, m3u8_list_lock_read);

	return this->m3u8_data_len;
}

