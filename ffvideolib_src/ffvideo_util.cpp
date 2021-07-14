

/////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdarg.h>

#include "ffvideo.h"

#include <boost/date_time/posix_time/posix_time.hpp>

////////////////////////////////////////////////////////////////////////////////
/* Windows sleep in 100ns units */
BOOLEAN nanosleep(LONGLONG ns) {
	/* Declarations */
	HANDLE timer;   /* Timer handle */
	LARGE_INTEGER li;   /* Time defintion */
	/* Create timer */
	if (!(timer = CreateWaitableTimer(NULL, TRUE, NULL)))
		return FALSE;
	/* Set timer properties */
	li.QuadPart = -ns;
	if (!SetWaitableTimer(timer, &li, 0, NULL, NULL, FALSE)) {
		CloseHandle(timer);
		return FALSE;
	}
	/* Start & wait for timer */
	WaitForSingleObject(timer, INFINITE);
	/* Clean resources */
	CloseHandle(timer);
	/* Slept without problems */
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
void FFVideo::ReportLog(const char* formatStr, ...)
{
	if (!mp_frameMgr->mp_stream_logging_callback)
		return;

	size_t bufsize = 1024 * 16;

	char* buffer = (char*)calloc(bufsize, 1); // 16K big enough for anything? in this app, I hope. 
	if (buffer != NULL)
	{
		va_list valist;
		va_start(valist, formatStr);
#ifdef WIN32
		vsprintf_s(buffer, bufsize, formatStr, valist);
#else
		vsprintf(buffer, formatStr, valist);
#endif
		va_end(valist);

		std::string destination(buffer);

		free(buffer);

		mp_frameMgr->mp_stream_logging_callback(destination.c_str(), mp_frameMgr->mp_stream_logging_object);
	}
}

//////////////////////////////////////////////////////////////////////////////////////
void FFVideo::replaceAll(std::string& str, const std::string& from, const std::string& to)
{
	if (from.empty())
		return;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
// depending upon the stream type, different playback options are enabled:
void FFVideo::setup_av_options(const char* cam_name, FFVIDEO_USB_Camera_Format* usb_format)
{
	// options for all stream types:
	av_dict_set(&mp_opts, "threads", "auto", 0);						// if multi-threading is needed, do it
	av_dict_set(&mp_opts, "refcounted_frames", "1", 0);		// ffplay sets this, so we do too
	av_dict_set(&mp_opts, "sync", "video", 0);
	av_dict_set(&mp_opts, "fflags", "discardcorrupt", 0);	// works, sets flag in mp_format_context

	switch (mp_frameMgr->m_stream_type)
	{
	case 0: // media file
		av_dict_set(&mp_opts, "framerate", "24", 0);
		av_dict_set(&mp_opts, "scan_all_pmts", "1", 0);		// ffplay uses this flag
		break;
	case 1:	// usb cam
		if (usb_format)
		{
			av_dict_set(&mp_opts, "fflags", "nobuffer", 0);

			// this is how a USB camera is requested, through the av options: 
			char bsjnk[1024];
			sprintf(bsjnk, "%dx%d", usb_format->m_max_width, usb_format->m_max_height);
			av_dict_set(&mp_opts, "video_size", bsjnk, 0);
			//
			sprintf(bsjnk, "%1.2f", usb_format->m_max_fps);
			av_dict_set(&mp_opts, "framerate", bsjnk, 0);
			//
			if (usb_format->m_pixelFormat.size() > 0)
			{
				av_dict_set(&mp_opts, "pixel_format", usb_format->m_pixelFormat.c_str(), 0);
			}
			else if (usb_format->m_pixelFormat.size() > 0)
			{
				av_dict_set(&mp_opts, "vcodec", usb_format->m_vcodec.c_str(), 0);
			}
		}
		else
		{
			av_dict_set(&mp_opts, "video_size", "640x480", 0);
			av_dict_set(&mp_opts, "framerate", "30", 0);
		}
		break;
	case 2: // ip cam
		av_dict_set(&mp_opts, "max_delay", "500000", 0);
		av_dict_set(&mp_opts, "rtsp_transport", "tcp", 0);
		av_dict_set(&mp_opts, "framerate", "29.97", 0);
		av_dict_set(&mp_opts, "allowed_media_types", "video", 0);
		break;
	}

	// necessary to pre-allocate, so we can set the I/O callback:
	mp_format_context = avformat_alloc_context();

	mp_format_context->interrupt_callback.callback = FFVideo_FrameMgr::interrupt_callback;
	mp_format_context->interrupt_callback.opaque = mp_frameMgr;

	if (mp_frameMgr->m_open_timeout != 0.0f)
		   mp_frameMgr->SetNextIOTimeout(mp_frameMgr->m_open_timeout);	// user set an open timeout, use that duration
	else mp_frameMgr->SetNextIOTimeout(12.0);													// no user timeout, allow 12.0 seconds to open
}

//////////////////////////////////////////////////////////////////////////////////////
void FFVideo::av_options_report(void)
{
	AVDictionaryEntry* de = NULL;
	while (de = av_dict_get(mp_opts, "", de, AV_DICT_IGNORE_SUFFIX))
	{
		av_log(mp_codec_context, AV_LOG_INFO, "FFVideo: requested option unused: '%s'.\n", de->key);
	}
	if (mp_codec_context)
	{
		const char* hwStat = "No";
		if (mp_codec_context->hwaccel)
			hwStat = "Yes,";
		av_log(mp_codec_context, AV_LOG_INFO, "%s hardware decompression in use.\n", hwStat);
	}
}

//////////////////////////////////////////////////////////////////////////////////////
int32_t FFVideo::check_stream_specifier(AVFormatContext* s, AVStream* st, const char* spec)
{
	int32_t ret = avformat_match_stream_specifier(s, st, spec);
	if (ret < 0)
		av_log(s, AV_LOG_ERROR, "Invalid stream specifier: %s.\n", spec);
	return ret;
}

//////////////////////////////////////////////////////////////////////////////////////
AVDictionary* FFVideo::filter_codec_opts(AVDictionary* opts, enum AVCodecID codec_id,
	AVFormatContext* s, AVStream* st, const AVCodec* codec)
{
	AVDictionary* ret = NULL;
	AVDictionaryEntry* t = NULL;
	int            flags = s->oformat ? AV_OPT_FLAG_ENCODING_PARAM
		: AV_OPT_FLAG_DECODING_PARAM;
	char          prefix = 0;
	const AVClass* cc = avcodec_get_class();

	if (!codec)
	{
		codec = s->oformat ? avcodec_find_encoder(codec_id) : avcodec_find_decoder(codec_id);
	}

	switch (st->codecpar->codec_type)
	{
	case AVMEDIA_TYPE_VIDEO:
		prefix = 'v';
		flags |= AV_OPT_FLAG_VIDEO_PARAM;
		break;
	case AVMEDIA_TYPE_AUDIO:
		prefix = 'a';
		flags |= AV_OPT_FLAG_AUDIO_PARAM;
		break;
	case AVMEDIA_TYPE_SUBTITLE:
		prefix = 's';
		flags |= AV_OPT_FLAG_SUBTITLE_PARAM;
		break;
	}

	while (t = av_dict_get(opts, "", t, AV_DICT_IGNORE_SUFFIX))
	{
		const AVClass* priv_class;
		char* p = strchr(t->key, ':');

		/* check stream specification in opt name */
		if (p)
			switch (check_stream_specifier(s, st, p + 1))
			{
			case  1: *p = 0; break;
			case  0:         continue;
			default:         break; //  exit_program(1);
			}

		if (av_opt_find(&cc, t->key, NULL, flags, AV_OPT_SEARCH_FAKE_OBJ) ||
			!codec ||
			((priv_class = codec->priv_class) && av_opt_find(&priv_class, t->key, NULL, flags, AV_OPT_SEARCH_FAKE_OBJ)))
		{
			av_dict_set(&ret, t->key, t->value, 0);
		}
		else if (t->key[0] == prefix && av_opt_find(&cc, t->key + 1, NULL, flags, AV_OPT_SEARCH_FAKE_OBJ))
		{
			av_dict_set(&ret, t->key + 1, t->value, 0);
		}

		if (p)
			*p = ':';
	}
	return ret;
}

//////////////////////////////////////////////////////////////////////////////////////
AVDictionary** FFVideo::setup_find_stream_info_opts(AVFormatContext* s, AVDictionary* codec_opts)
{
	uint32_t i;
	AVDictionary** opts;

	if (!s->nb_streams)
		return NULL;

	opts = (AVDictionary**)av_mallocz_array(s->nb_streams, sizeof(*opts));
	if (!opts)
	{
		av_log(NULL, AV_LOG_ERROR, "Could not alloc memory for stream options.\n");
		return NULL;
	}

	for (i = 0; i < s->nb_streams; i++)
		opts[i] = filter_codec_opts(codec_opts, s->streams[i]->codecpar->codec_id, s, s->streams[i], NULL);

	return opts;
}


//////////////////////////////////////////////////////////////////////////////////////
void FFVideo::SetPostProcessFilter( std::string& filter ) 
{ 
	std::unique_lock<std::shared_mutex> lock(m_post_process_lock);
	m_post_process = filter; 
	lock.unlock();
}

//////////////////////////////////////////////////////////////////////////////////////
bool FFVideo::SetFrameExportingParams(int32_t export_interval, std::string& export_dir, std::string& export_base, 
																			float scale, int32_t quality, 
																			 EXPORT_FRAME_CALLBACK_CB frame_export_cb, void* frame_export_object)
{
	if (!mp_frameMgr)
     return false;

	return mp_frameMgr->SetFrameExporting(export_interval, export_dir, export_base, scale, quality, frame_export_cb, frame_export_object);
}

//////////////////////////////////////////////////////////////////////////////////////
void FFVideo::GetFrameExportingParams(int32_t& export_interval, std::string& export_dir, std::string& export_base, 
																			float& scale, int32_t& quality)
{
	if (!mp_frameMgr)
	{
		export_interval = -1;
		return;
  }
	export_interval = mp_frameMgr->mp_frame_dest->m_frame_export_interval;
	export_dir      = mp_frameMgr->mp_frame_dest->m_export_dir;
	export_base			= mp_frameMgr->mp_frame_dest->m_export_base;

	scale = mp_frameMgr->mp_frame_dest->m_export_scale;
	quality = mp_frameMgr->mp_frame_dest->m_export_quality;
}

//////////////////////////////////////////////////////////////////////////////////////
bool FFVideo::GetFrameExportingError(void)
{
	if (!mp_frameMgr)
		return false;

	return (mp_frameMgr->mp_frame_dest->m_frame_export_interval == -1);
}

//////////////////////////////////////////////////////////////////////////////////////
int32_t FFVideo::GetFrameExportingQueueSize(void)
{
	if (!mp_frameMgr)
		return -1;

	return (int32_t)mp_frameMgr->mp_frame_dest->m_frame_exporter.Size();
}

//////////////////////////////////////////////////////////////////////////////////////
void FFVideo_FrameExporter::Add(FFVideo_Image& im, int32_t frame_num, int32_t export_num)
{
	using namespace boost::posix_time;
	ptime t = microsec_clock::universal_time();
	//
	std::string iso_part = to_iso_extended_string(t);

	replaceAll(iso_part, std::string(":"), std::string("-"));
	replaceAll(iso_part, std::string("."), std::string("-"));

  iso_part = std::string("_") + iso_part;

	std::string outfile = mp_parent->m_export_dir + mp_parent->m_export_base + iso_part;

	outfile += std::string(".jpg");

	FFVideo_ExportFrame ef;
	ef.m_fname = outfile;
	ef.m_im.Clone(im);
	ef.m_frame_num = frame_num;
	ef.m_export_num = export_num;

	std::unique_lock<std::shared_mutex> lock(m_queue_lock);
		m_exportQue.push(ef);
	lock.unlock();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// no trailing slash allowed
int32_t FFVideo_FrameExporter::IsDirectory(const char* path)
{
	struct stat info;

	int32_t ret = stat(path, &info);

	if (ret < 0)
	{
		if (errno == ENOENT)
		{
			// does not exist:
			return 0;
		}

		// if (errno == EINVAL)
		// bad input?
		return -1;
	}
	else if (info.st_mode & S_IFDIR)  
		return 1; // is a directory
	else
		return 0; // not a directory
}
