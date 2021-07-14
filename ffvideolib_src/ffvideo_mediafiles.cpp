

/////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdarg.h>

#include "ffvideo.h"

//////////////////////////////////////////////////////////////////////////////////////
bool FFVideo::CalcTimestampParts(int64_t& ts, int32_t& hours, int32_t& minutes, int32_t& seconds)
{
	if (ts <= 0)
	{
		ReportLog("negative timestamp is not allowed");
		return false;
	}

	int64_t tns;

	tns = ts / 1000000LL;

	hours = (int)(tns / 3600);
	minutes = (int)((tns % 3600) / 60);
	seconds = (int)(tns % 60);

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////
bool FFVideo::CalcTimestampFromNormalizedPosition(int64_t& ts, double normTime)
{
	if (!mp_format_context)
	{
		ReportLog("format_context is NULL");
		return false;
	}
	//
	if (mp_frameMgr->m_stream_type != 0) // must be media file file to know duration
	{
		ReportLog("stream is not media file, illegal use of function");
		return false;
	}
	//
	if (mp_format_context->duration <= 0)
	{
		ReportLog("media file duration is yet unknown");
		return false;
	}

	// clamp to the normalized range:
	if (normTime < 0.0) normTime = 0.0;
	else if (normTime > 1.0) normTime = 1.0;

	// generate the timestamp:
	ts = (int64_t)((double)normTime * mp_format_context->duration);

	// possible offset for non-zero stream start time: 
	if (mp_format_context->start_time != AV_NOPTS_VALUE)
		ts += mp_format_context->start_time;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////
bool FFVideo::GetDuration(int64_t& ts)
{
	if (!mp_format_context)
	{
		ReportLog("format_context is NULL");
		return false;
	}
	//
	if (mp_frameMgr->m_stream_type != 0) // must be media file file to know duration
	{
		ReportLog("stream is not media file, illegal use of function");
		return false;
	}
	//
	if (mp_format_context->duration <= 0)
	{
		ReportLog("media file duration is yet unknown");
		return false;
	}

	ts = mp_format_context->duration;

	if (mp_format_context->start_time != AV_NOPTS_VALUE)
		ts += mp_format_context->start_time;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////
bool FFVideo::GetDurationSeconds(double& duration)
{
	if (!mp_format_context)
	{
		ReportLog("format_context is NULL");
		return false;
	}
	//
	if (mp_frameMgr->m_stream_type != 0) // must be media file file to know duration
	{
		ReportLog("stream is not media file, illegal use of function");
		return false;
	}
	//
	if (mp_format_context->duration <= 0)
	{
		ReportLog("media file duration is still unknown");
		return false;
	}

	int64_t ts = mp_format_context->duration;

	if (mp_format_context->start_time != AV_NOPTS_VALUE)
		ts += mp_format_context->start_time;

	// convert to seconds:
	duration = (double)ts / (double)AV_TIME_BASE;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////
bool FFVideo::SeekRelative(double offset)
{
	if (mp_frameMgr->m_stream_type != 0) // must be file to seek
	{
		ReportLog("stream is not media file, illegal use of function");
		return false;
	}

	if (mp_frameMgr->m_seek_by_bytes)
	{
		int64_t pos = -1;
		if (mp_frameMgr->m_seek_anchor >= 0)
			pos = mp_frameMgr->m_seek_anchor;
		if (pos < 0)
			pos = avio_tell(mp_format_context->pb);

		if (mp_format_context->bit_rate)
			offset *= mp_format_context->bit_rate / 8.0;
		else offset *= 180000.0;

		return Seek(pos, (int64_t)offset, 1);
	}
	else
	{
		// calc the current position as seconds:
		double	pos = (double)mp_frameMgr->m_seek_anchor * m_timebase;

		pos += offset;	// just add our passed offset

		if (mp_format_context->start_time != AV_NOPTS_VALUE && pos < mp_format_context->start_time / (double)AV_TIME_BASE)
			pos = mp_format_context->start_time / (double)AV_TIME_BASE;

		// converting seconds to timestamps: 
		return Seek((int64_t)(pos * AV_TIME_BASE), (int64_t)(offset * AV_TIME_BASE), 0);
	}
}

//////////////////////////////////////////////////////////////////////////////////////
bool FFVideo::SeekNormalized(double normalized_position)
{
	// 3 ways to fail, see the function if know how: 
	int64_t ts;
	if (CalcTimestampFromNormalizedPosition(ts, normalized_position) == false)
		return false;

	// make sure no other seek is already running: 
	if (mp_frameMgr->m_seek_req)
	{
		ReportLog("Previous Seek() is not completed.");
		return false;
	}

	return Seek(ts, 0, 0);
}

//////////////////////////////////////////////////////////////////////////////////////
bool FFVideo::SeekToFrame(uint32_t frame_number)
{
	int64_t ts;
	if (!GetDuration(ts))	// sets e_msg if returning false
		return false;

	if (m_expected_frame_rate <= 0.0)
	{
		ReportLog("bad expected frame rate, corrupt media file?");
		return false;
	}

	// convert to seconds:
	double seconds = frame_number / m_expected_frame_rate;

	// convert to timestamp:
	int64_t pos = (int64_t)(seconds * AV_TIME_BASE);

	// if requesting a frame past the end of the file, set to last frame: 
	if (pos > ts)
		pos = ts;

	return Seek(pos, 0, 0);
}

//////////////////////////////////////////////////////////////////////////////////////
bool FFVideo::Seek(int64_t pos, int64_t rel, bool seek_by_bytes)
{
	if (mp_frameMgr->m_stream_type != 0) // must be file to seek
	{
		ReportLog("stream is not media file, illegal use of function");
		return false;
	}

	// only seek if no other seek is already active:
	if (!mp_frameMgr->m_seek_req)
	{
		mp_frameMgr->m_seek_pos = pos;
		mp_frameMgr->m_seek_rel = rel;
		mp_frameMgr->m_seek_flags &= ~AVSEEK_FLAG_BYTE;
		if (seek_by_bytes)
			mp_frameMgr->m_seek_flags |= AVSEEK_FLAG_BYTE;
		mp_frameMgr->m_seek_req = true;

		return true;
	}

	ReportLog("Previous Seek() is not completed.");
	return false;
}
