

/////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdarg.h>

#include "ffvideo.h"

//////////////////////////////////////////////////////////////////////////////////////
bool FFVideo::StartUSB(FFVIDEO_USB_Camera_Format* camera)
{
	if (!camera)
	{
		return StartUSBGuessingCamera();
	}

	// USB cams need the "direct show" input format:
	const AVInputFormat* input_format = av_find_input_format("dshow");

	m_vid_src = std::string("video=") + camera->m_name;
	const char* finalSrcStr = m_vid_src.c_str();

	FFVIDEO_USB_Camera_Format* active_usb_format = camera;
	av_log(mp_codec_context, AV_LOG_INFO, "ChoosingUSB: %dx%d @ %1.2f fps\n",
		active_usb_format->m_max_width, active_usb_format->m_max_height, active_usb_format->m_max_fps);

	av_log_set_callback(NULL);

	assert(mp_format_context == NULL); // if not NULL, a previous stream was left open

	av_log(mp_codec_context, AV_LOG_INFO, "StartUSB: %s\n", finalSrcStr);

	// adds things to the option dictionary based upon stream type
	// also allocates mp_format_context to set an I/O callback
	setup_av_options(camera->m_name.c_str(), active_usb_format);

	// mp_format_context will be set to a valid AVFormatContext if avformat_open_input() succeeeds
	if (avformat_open_input(&mp_format_context, finalSrcStr, input_format, &mp_opts) != 0)
	{
		ReportLog("Could not open camera at USB Pin %d.", m_usb_pin);
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////
bool FFVideo::StartUSBGuessingCamera(void)
{
	// get the names of the USB cameras so we can identify the one we want:
	std::vector<std::string> usb_names;
	GetUSB_CameraNames(usb_names);

	// the USB Pin in the index into these USB device names, make sure index is valid:
	if (usb_names.size() > 0 && m_usb_pin < usb_names.size())
	{
		// USB cams need the "direct show" input format:
		const AVInputFormat* input_format = av_find_input_format("dshow");

		// the cam is referenced in a string: video=[device name] where the "device name"
		// came from GetUSB_CameraNames(), and the USB Pin is the index to each:
		std::string cameraStr = usb_names[m_usb_pin];
		m_vid_src = std::string("video=") + cameraStr;
		const char* finalSrcStr = m_vid_src.c_str();

		std::vector<FFVIDEO_USB_Camera_Format> usb_formats;
		GetCameraPixelFormats(cameraStr.c_str(), usb_formats);

		// select a USB Format to use:
		int pix_format = -1;
		for (int i = 0; i < usb_formats.size(); i++)
		{
			if (i == 0) pix_format = 0; // default to whatever the first one is
			else
			{
				int curr_rank = -1, new_rank = -1;

				if (strcmp(usb_formats[pix_format].m_pixelFormat.c_str(), "pixel_format=bgr24") == 0)
					curr_rank = 2;
				else if (strcmp(usb_formats[pix_format].m_pixelFormat.c_str(), "pixel_format=bgr32") == 0)
					curr_rank = 1;
				else if (strcmp(usb_formats[pix_format].m_pixelFormat.c_str(), "vcodec=mjpeg") == 0)
					curr_rank = 1;
				else if (strcmp(usb_formats[pix_format].m_pixelFormat.c_str(), "pixel_format=yuyv422") == 0)
					curr_rank = 0;

				if (strcmp(usb_formats[i].m_pixelFormat.c_str(), "pixel_format=bgr24") == 0)
					new_rank = 2;
				else if (strcmp(usb_formats[i].m_pixelFormat.c_str(), "pixel_format=bgr32") == 0)
					new_rank = 1;
				else if (strcmp(usb_formats[i].m_pixelFormat.c_str(), "vcodec=mjpeg") == 0)
					new_rank = 1;
				else if (strcmp(usb_formats[i].m_pixelFormat.c_str(), "pixel_format=yuyv422") == 0)
					new_rank = 0;

				if (new_rank > curr_rank)
				{
					pix_format = i;
				}
				else if (new_rank == curr_rank)
				{
					int area_cur = usb_formats[pix_format].m_max_width * usb_formats[pix_format].m_max_height;
					int area_new = usb_formats[i].m_max_width * usb_formats[i].m_max_height;
					if ((area_cur < area_new) && (usb_formats[pix_format].m_max_fps <= usb_formats[i].m_max_fps))
					{
						pix_format = i;
					}
					else if (area_cur == area_new)
					{
						if (usb_formats[pix_format].m_max_fps < usb_formats[i].m_max_fps)
						{
							pix_format = i;
						}
					}
				}
			}
		}


		FFVIDEO_USB_Camera_Format* active_usb_format = NULL;
		if (pix_format != -1)
		{
			active_usb_format = &usb_formats[pix_format];

			av_log(mp_codec_context, AV_LOG_INFO, "ChoosingUSB: %dx%d @ %1.2f fps\n",
				active_usb_format->m_max_width, active_usb_format->m_max_height, active_usb_format->m_max_fps);
		}

		assert(mp_format_context == NULL); // if not NULL, a previous stream was left open

		av_log(mp_codec_context, AV_LOG_INFO, "StartUSB: %s\n", finalSrcStr);

		// adds things to the option dictionary based upon stream type, also allocates mp_format_context to set an I/O callback
		setup_av_options(cameraStr.c_str(), active_usb_format);

		// mp_format_context will be set to a valid AVFormatContext if avformat_open_input() succeeeds
		if (avformat_open_input(&mp_format_context, finalSrcStr, input_format, &mp_opts) != 0)
		{
			ReportLog("Could not open camera at USB Pin %d.", m_usb_pin);
			return false;
		}

		return true;
	}

	ReportLog("USB Pin %d is not valid.", m_usb_pin);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////

static std::atomic<bool>        gFFVideoCaptureLogFlag(false);
static std::vector<std::string> gFFVideoCapturedLog;

//////////////////////////////////////////////////////////////////////////////////////
bool FFVideo::is_query_capture_on( void )
{
	bool ret = gFFVideoCaptureLogFlag;
	return ret;
}

//////////////////////////////////////////////////////////////////////////////////////
void FFVideo::start_query_capture(void)
{
	gFFVideoCaptureLogFlag = true;
	gFFVideoCapturedLog.clear();
	av_log_set_callback(query_callback);
}

//////////////////////////////////////////////////////////////////////////////////////
void FFVideo::query_capture(const char* message)
{
	// if requested, remember:
	if (gFFVideoCaptureLogFlag)
	{
		if (strncmp(message, "real-time buffer", 16) != 0)
		{
			gFFVideoCapturedLog.push_back(message);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////
void FFVideo::stop_query_capture(std::vector<std::string>& captured_log)
{
	av_log_set_callback(logging_callback);

	captured_log.clear();

	gFFVideoCaptureLogFlag = false;

	size_t lim = gFFVideoCapturedLog.size();

	for (size_t i = 0; i < lim; i++)
		captured_log.push_back(gFFVideoCapturedLog[i].c_str());

	gFFVideoCapturedLog.clear();
}

//////////////////////////////////////////////////////////////////////////////////////
void FFVideo::query_callback(void* ptr, int level, const char* fmt, va_list vargs)
{
	static char message[8192];

	std::string std_fmt = std::string(fmt);

	// get rid of unknown format descriptors from h264 module:
	replaceAll(std_fmt, std::string("%td"), std::string("%d"));

	// Create the actual message
#ifdef WIN32
	vsnprintf_s(message, sizeof(message), _TRUNCATE, std_fmt.c_str(), vargs);
#else
	vsnprintf(message, sizeof(message), std_fmt.c_str(), vargs);
#endif

	// capture of logging output, for when pixel formats are queried
	query_capture(message); // not bothering to send repeated line in last_message
}


#ifdef WIN32
//////////////////////////////////////////////////////////////////////////////////////
// need to link to ole32.lib
#include <dshow.h>
#pragma comment(lib, "strmiids.lib")
HRESULT EnumerateDevices(REFGUID category, IEnumMoniker** ppEnum)
{
	// Create the System Device Enumerator.
	ICreateDevEnum* pDevEnum;
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL,
		CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDevEnum));
	if (SUCCEEDED(hr))
	{
		// Create an enumerator for the category.
		hr = pDevEnum->CreateClassEnumerator(category, ppEnum, 0);
		if (hr == S_FALSE)
		{
			hr = VFW_E_NOT_FOUND;  // The category is empty. Treat as an error.
		}
		pDevEnum->Release();
	}
	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////
void AddUSBcameraName(IEnumMoniker* pEnum, std::vector<std::string>& names)
{
	IMoniker* pMoniker = NULL;

	while (pEnum->Next(1, &pMoniker, NULL) == S_OK)
	{
		IPropertyBag* pPropBag;
		HRESULT hr = pMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag));
		if (FAILED(hr))
		{
			pMoniker->Release();
			continue;
		}

		VARIANT var;
		VariantInit(&var);

		// Get description or friendly name.
		bool is_description = true;
		hr = pPropBag->Read(L"Description", &var, 0);
		if (FAILED(hr))
		{
			hr = pPropBag->Read(L"FriendlyName", &var, 0);
			is_description = false;
		}
		if (SUCCEEDED(hr))
		{
			char buf[1024];
			sprintf(buf, "%S", var.bstrVal); // NOTE! using S not s to print the BSTR
			names.push_back(std::string(buf));
			VariantClear(&var);
		}

		pPropBag->Release();
		pMoniker->Release();
	}
}

//////////////////////////////////////////////////////////////////////////////////////
// This works for DirectShow, it gets the USB camera names into a list. 
// Use this on Windows to identify which camera depending on the USB pin the user gives; 
// the "pin" is the index into the list of names, selecting the index'ed usb device for input. 
bool FFVideo::GetUSB_CameraNames(std::vector<std::string>& usb_names)
{
	usb_names.clear();

	HRESULT hr = CoInitialize(NULL);
	if (SUCCEEDED(hr))
	{
		IEnumMoniker* pEnum;

		hr = EnumerateDevices(CLSID_VideoInputDeviceCategory, &pEnum);
		if (SUCCEEDED(hr))
		{
			AddUSBcameraName(pEnum, usb_names);
			pEnum->Release();
		}
		CoUninitialize();
	}
	else return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////
bool FFVideo::GetCameraPixelFormats(const char* cameraName, std::vector<FFVIDEO_USB_Camera_Format>& usb_formats)
{
	// USB cams need the "direct show" input format:
	const AVInputFormat* input_format = av_find_input_format("dshow");

	std::string usb_video_src = std::string("video=") + cameraName;
	const char* finalSrcStr = usb_video_src.c_str();

	// this "opens" a video stream, but actually is a signal to the underlying ffmpeg engine
	// to output the named USB camera's available formats. The output to this goes to the log,
	// so here we clear a log retention buffer, and tell the lib to start retaining log msgs:

	AVFormatContext* pFormatCtx = avformat_alloc_context();
	AVDictionary* options = NULL;
	av_dict_set(&options, "list_options", "true", 0);
	//
	start_query_capture();
	//
	// generate pixel format log messages
	avformat_open_input(&pFormatCtx, finalSrcStr, input_format, &options);
	//
	std::vector<std::string> captured_log;
	stop_query_capture(captured_log);

	// process:
	usb_formats.clear();
	bool foundStart = false, foundEnd = false, good = true;
	int  nTokens;
	char line[1024], * token, * parseTokens[64], * seps = " ,\t\"\n\r", * nextToken = NULL;
	FFVIDEO_USB_Camera_Format* p_usb_format = NULL;
	for (uint32_t i = 0; i < captured_log.size() && !foundEnd; i++)
	{
		if (captured_log[i].size() < 1024)
		{
			strcpy(line, captured_log[i].c_str());

			// when this is completed 'line' will be a series of NULL terminated strings, each pointed to by an element of parseTokens[]:
			nTokens = 0;
			token = strtok_s(line, seps, &nextToken);
			while (token != NULL)
			{
				parseTokens[nTokens++] = token;
				token = strtok_s(NULL, seps, &nextToken);
			}

			if (nTokens == 7)
			{
				if ((strcmp(parseTokens[0], "DirectShow") == 0) &&
					(strcmp(parseTokens[1], "video") == 0) &&
					(strcmp(parseTokens[2], "device") == 0) &&
					(strcmp(parseTokens[3], "options") == 0))
				{
					if (!foundStart)
					{
						foundStart = true;
						continue;
					}
					else good = false;	// the start code was encountered twice - two cameras are querying at same time. 
				}
				else if ((strcmp(parseTokens[0], "Pin") == 0) &&
					(strcmp(parseTokens[1], "Capture") == 0) &&
					(strcmp(parseTokens[2], "(alternative") == 0) &&
					(strcmp(parseTokens[3], "pin") == 0) &&
					(strcmp(parseTokens[4], "name") == 0))
				{
					// an expected line we're ignoring:
					continue;
				}
			}
			else if (nTokens == 4)
			{
				if (strcmp(parseTokens[0], "StartUSB:") == 0)
				{
					if (!foundEnd)
					{
						foundEnd = true;
						continue;
					}
					else continue; // ?? good = false;	// the end code was encountered twice - two cameras are querying at same time. 
				}
			}
			else if (nTokens == 5)
			{
				if ((strcmp(parseTokens[0], "Selecting") == 0) &&
					(strcmp(parseTokens[1], "pin") == 0) &&
					(strcmp(parseTokens[2], "Capture") == 0) &&
					(strcmp(parseTokens[3], "on") == 0) &&
					(strcmp(parseTokens[4], "video") == 0))
				{
					if (!foundEnd)
					{
						foundEnd = true;
						continue;
					}
					else continue; // ?? good = false;	// the end code was encoutered twice - two cameras are querying at same time. 
				}
			}

			if (good && foundStart && !foundEnd)
			{
				if (nTokens == 6)
				{
					if ((strcmp(parseTokens[0], "min") == 0) &&
						(strncmp(parseTokens[1], "s=", 2) == 0) &&
						(strncmp(parseTokens[2], "fps=", 4) == 0) &&
						(strcmp(parseTokens[3], "max") == 0) &&
						(strncmp(parseTokens[4], "s=", 2) == 0) &&
						(strncmp(parseTokens[5], "fps=", 4) == 0))
					{
						if (!p_usb_format)
						{
							// unrecognized pixel format:
							continue;
						}
						else
						{
							int num = sscanf(parseTokens[1], "s=%dx%d", &p_usb_format->m_min_width, &p_usb_format->m_min_height);
							if (num != 2)
								good = false;

							if (good)
							{
								num = sscanf(parseTokens[2], "fps=%f", &p_usb_format->m_min_fps);
								if (num != 1)
									good = false;
							}

							if (good)
							{
								num = sscanf(parseTokens[4], "s=%dx%d", &p_usb_format->m_max_width, &p_usb_format->m_max_height);
								if (num != 2)
									good = false;
							}

							if (good)
							{
								num = sscanf(parseTokens[5], "fps=%f", &p_usb_format->m_max_fps);
								if (num != 1)
									good = false;
							}

							if (good)
							{
								usb_formats.push_back(*p_usb_format);
								// now p_usb_format's memory is managed by usb_format, so
								p_usb_format = NULL;
							}
						}
					}
				}
				else if (nTokens == 1)
				{
					int what = 0;
					if (strncmp(parseTokens[0], "pixel_format=", 13) == 0)
						what = 1;
					else if (strncmp(parseTokens[0], "vcodec=", 7) == 0)
						what = 2;

					if (what > 0)
					{
						if (!p_usb_format)
						{
							p_usb_format = new FFVIDEO_USB_Camera_Format();
						}

						if (what == 1)
						{
							p_usb_format->m_pixelFormat = std::string(&parseTokens[0][13]);
						}
						else
						{
							p_usb_format->m_vcodec = std::string(&parseTokens[0][7]);
						}
					}
				}
				else if (nTokens == 4)
				{
					if ((strcmp(parseTokens[0], "unknown") == 0) &&
						(strcmp(parseTokens[1], "compression") == 0) &&
						(strcmp(parseTokens[2], "type") == 0))
					{
						if (p_usb_format)
						{
							good = false; // encountered new pixel format before resolutions
						}
						else
						{
							p_usb_format = new FFVIDEO_USB_Camera_Format();
							p_usb_format->m_pixelFormat = std::string("unknown");
						}
					}
				}
			}
		}
	}

	for (int32_t j = 0; j < (int32_t)usb_formats.size(); j++)
	{
		usb_formats[j].m_name = std::string(cameraName);
	}

	return good;
}

#else
//////////////////////////////////////////////////////////////////////////////////////
bool FFVideo::GetUSB_CameraNames(std::vector<std::string>& usb_names)
{
	// no need for this as the linux USB cameras are labelled
	// /dev/video0  /dev/video1 etc.
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////
bool FFVideo::GetCameraPixelFormats(const char* cameraName, std::vector<FFVIDEO_USB_Camera_Format>& usb_formats)
{
	return false;
}
#endif

