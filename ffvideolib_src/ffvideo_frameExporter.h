#pragma once
#ifndef _FFVIDEO_FRAMEEXPORTER_H_
#define _FFVIDEO_FRAMEEXPORTER_H_


#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <functional>


extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "libavutil/error.h"
#include "libavfilter/avfilter.h"
#include "libavutil/log.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/avstring.h"
}
#pragma comment(lib, "libavformat.a")

#include "BCTime.h"
#include "ffvideo_image.h"


//------------------------------------------------------------------------------
class FFVideo_ExportFrame
{
public:
	FFVideo_ExportFrame() {};

	// copy constructor 
	FFVideo_ExportFrame(const FFVideo_ExportFrame& ef)
	{
		m_im.Clone(ef.m_im);
		m_fname = ef.m_fname;
		m_frame_num = ef.m_frame_num;
		m_export_num = ef.m_export_num;
	}

	// copy assignement operator
	FFVideo_ExportFrame& operator = (const FFVideo_ExportFrame& ef)
	{
		if (this != &ef)
		{
			m_im.Clone(ef.m_im);
			m_fname = ef.m_fname;
			m_frame_num = ef.m_frame_num;
			m_export_num = ef.m_export_num;
		}
		return (*this);
	}

	FFVideo_Image	m_im;
	std::string		m_fname;
	int32_t				m_frame_num;
	int32_t				m_export_num;
};

class FFVideo_FrameDestination;

// the "export frame callback" is called with every frame export
typedef void(*EXPORT_FRAME_CALLBACK_CB)(void* p_object, int32_t frame_num, int32_t export_num, const char* filepath, bool status);

//------------------------------------------------------------------------------
class FFVideo_FrameExporter
{
public:
	FFVideo_FrameExporter() : mp_exportProcessingThread(NULL), m_stop_export_processing_loop(false), 
		m_export_processing_loop_ended(false), mp_parent(NULL), mp_export_frame_cb(NULL), mp_export_frame_object(NULL) {};

	// 2nd required for for thread constructor
	FFVideo_FrameExporter(const FFVideo_FrameExporter& obj) {}

	// class sub-thread function that spins writing video frames to jpgs:
	void ExportProcessLoop(void);
	//
	// thread variables:
	std::thread* mp_exportProcessingThread;
	//
	std::atomic<bool>		m_stop_export_processing_loop;
	std::atomic<bool>		m_export_processing_loop_ended;

	~FFVideo_FrameExporter()
	{
		StopExporter();
		std::queue<FFVideo_ExportFrame> empty;
		std::swap(m_exportQue, empty);
	}

	bool IsRunning(void)
	{
		if (!mp_exportProcessingThread)
			return false;

		// set to true entering Process thread, goes false when exiting Process:
		bool ret = !m_export_processing_loop_ended;

		return ret;
	}

	void StartExporter(void)
	{
		if (!IsRunning())
		{
			mp_exportProcessingThread = new std::thread(&FFVideo_FrameExporter::ExportProcessLoop, this);
		}
	}

	void StopExporter(void)
	{
		// only if the ExportProcessLoop() is running:
		if (IsRunning())
		{
			// tell VideoProcessLoop() (running in it's own thread) to exit:
			m_stop_export_processing_loop = true;
			//
			uint32_t spins = 0;
			while (!m_export_processing_loop_ended)
			{
				using namespace std::chrono_literals;
				std::this_thread::sleep_for(200ms);
				//
				spins++;
			}
			mp_exportProcessingThread->join();
			delete mp_exportProcessingThread;
			mp_exportProcessingThread = NULL;
			m_stop_export_processing_loop = false; // reset for next use
			m_export_processing_loop_ended = false;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	void replaceAll(std::string& str, const std::string& from, const std::string& to)
	{
		if (from.empty())
			return;
		size_t start_pos = 0;
		while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
			str.replace(start_pos, from.length(), to);
			start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	int32_t IsDirectory(const char* path);

	//////////////////////////////////////////////////////////////////////////////////////
	// gens filename w/ ISO timecode including milliseconds for time called & adds to queue
	void Add(FFVideo_Image& im, int32_t frame_num, int32_t export_num);

	//////////////////////////////////////////////////////////////////////////////////////
	size_t Size(void) {	
		std::shared_lock<std::shared_mutex> rlock(m_queue_lock);
		size_t work_to_do = m_exportQue.size();
		rlock.unlock();
		return work_to_do;
	}

	FFVideo_FrameDestination*				mp_parent;

	// the "export frame callback" is called with every frame export
	// typedef void(*EXPORT_FRAME_CALLBACK_CB)(void* p_object, int32_t frame_num, const char* filepath, bool status);
	//
	EXPORT_FRAME_CALLBACK_CB		mp_export_frame_cb;
	void*												mp_export_frame_object;

	mutable std::shared_mutex				m_queue_lock;
	std::queue<FFVideo_ExportFrame>	m_exportQue;
};



#endif // _FFVIDEO_FRAMEEXPORTER_H_
