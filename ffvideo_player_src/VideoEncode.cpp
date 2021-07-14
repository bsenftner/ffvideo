///////////////////////////////////////////////////////////////////////////////
// Name:        VideoEncode.cpp
// Author:			Blake Senftner
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "ffvideo_player_app.h"


/////////////////////////////////////////////////////////////////////////
void RenderCanvas::OnVideoEncode(wxCommandEvent& event)
{
	if (!mp_app) 
		return;
	if (!mp_app->m_we_are_launched || m_terminating)
		return;

	VideoStreamConfig* vsc = mp_videoWindow->mp_streamConfig;

	// create new instance of a FrameEncoder:
	std::unique_lock lock(m_encodersSharedMutex);	// lock from other threads changing m_encoders (the vector)
	m_encoders.emplace_back( new FrameEncoder() );
	lock.unlock();

	FrameEncoder* fe = m_encoders.back();

	fe->mp_parent = this;
	fe->mp_app    = mp_app;

	fe->mp_thread = NULL;
	fe->m_threadStarted = false;
	fe->m_threadExited  = false;
	fe->m_threadJoined  = false;
	//
	// m_next_encoder_id has advanced one by the time this runs:
	fe->m_id									= m_next_encoder_id - 1; 
	//
	fe->m_move_errors         = 0;
	fe->m_final_move_errors   = 0;
	fe->m_final_delete_errors = 0;
	//
	// get the vector of exported image file paths:
	fe->m_filepaths = m_exportedFramePaths[ fe->m_id % 2 ];
	//
	// needs to be cleared for next use:
	m_exportedFramePaths[ fe->m_id % 2 ].clear();
	//
	fe->m_export_dir     = vsc->m_export_dir;
	fe->m_export_base    = vsc->m_export_base;
	//
	fe->m_encode_interval = vsc->m_encode_interval.load(std::memory_order::memory_order_relaxed);
	fe->m_post_encode     = vsc->m_post_encode;
	fe->m_ffmpeg_path     = vsc->m_ffmpeg_path;
	fe->m_post_move_path  = vsc->m_post_move_path;
	fe->m_encode_dir      = vsc->m_encode_dir;
	fe->m_encode_base     = vsc->m_encode_base;
	fe->m_encode_fps      = vsc->m_encode_fps;
	fe->m_encode_type     = vsc->m_encode_type;
	fe->m_encode_width    = vsc->m_encode_width;
	fe->m_encode_height   = vsc->m_encode_height;

	fe->mp_thread = new std::thread(&FrameEncoder::EncodeFrames, fe);
}

/////////////////////////////////////////////////////////////////////////
void FrameEncoder::EncodeFrames(void)
{
	using namespace std::chrono_literals;

	m_threadStarted = true;

	// should never happen, but wtf happens sometimes
	if (m_filepaths.size() > 0)
	{
		// the encode source files are each named basename_timecode.jpg, let's use the first file as the template for any output files:
		std::string export_timefrag = mp_app->GetFilename( m_filepaths[0] );
		std::vector<std::string> parts = mp_app->split( export_timefrag, "_" );
		export_timefrag = parts[1];

		// first create a text file containing the image encoding files:
		std::string list_filename = m_encode_dir + "\\" + m_encode_base + mp_app->FormatStr("_%d_%s.txt", m_id, export_timefrag.c_str() );
		FILE *fp = fopen( list_filename.c_str(), "w" );
		for (std::size_t i = 0; i < m_filepaths.size(); i++)
		{
			std::string list_version =  m_filepaths[i];

			mp_app->stdReplaceAll( list_version, std::string("\\"), std::string("/") );

			std::string msg = std::string("file ") + list_version + "\n";
			fputs( msg.c_str(), fp );
		}
		fclose(fp);
		//
		// call ffmpeg like this, generating a conventional media player mp4:
		// ffmpeg.exe -f image2 -safe 0 -f concat -i list.txt -vf scale=1024:-1 -r 25 -c:v libx264 -pix_fmt yuv420p -refs 16 -crf 18 -preset ultrafast output3.mp4 
		//

		//
		// build encode mp4 filepath
		std::string mp4outfile = m_encode_dir + std::string("\\") + m_encode_base + mp_app->FormatStr("_%d_%s.mp4", m_id, export_timefrag.c_str() );
		//
		// build full filepath for a stdout capture file
		std::string stdoutfile = m_encode_dir + std::string("\\") + m_encode_base + mp_app->FormatStr("_%d_%s_report.txt", m_id, export_timefrag.c_str() );
		// 
		std::string args = mp_app->FormatStr( 
			"-f image2 -safe 0 -f concat -i %s -vf scale=%d:%d -r %1.2f -c:v libx264 -pix_fmt yuv420p -refs 16 -crf 18 -preset ultrafast %s > %s 2>&1",
			list_filename.c_str(), m_encode_width, m_encode_height, m_encode_fps, mp4outfile.c_str(), stdoutfile.c_str() );
		/*
		windows_process wp;
		std::string     err;
		//
		PROCESS_INFORMATION pi = wp.launchProcess( m_ffmpeg_path, args, err );
		//
		// give a half second for ffmpeg to begin, maybe even complete: 
		std::this_thread::sleep_for(500ms);
		//
		bool ffmpeg_failed(false);
		for( ;; )
		{
		if (wp.checkIfProcessIsActive( pi, err ))
		std::this_thread::sleep_for(100ms);
		else
		{
		if (err.size() > 0)
		{
		ffmpeg_failed = true;
		}
		// ffmpeg is done:
		break;
		}
		} */
		std::string cmd = m_ffmpeg_path + " " + args;
		int32_t ret = system( cmd.c_str() );
		if (ret == -1)
		{
			ret = errno;
			mp_app->ReportLog( ReportLogOp::flush, mp_app->FormatStr( "FFmpeg error %d\n", ret ) );

			if (m_encode_type == 1)
			{
				mp_app->ReportLog( ReportLogOp::flush, mp_app->FormatStr( "Due to FFmpeg error, elementary stream encoding is canceled\n" ) );
			}
		}
		else if (m_encode_type == 1)
		{
			// we're encoding an elementary stream too:
			// need a cmd like this:
			//		ffmpeg -i [inputfile.mp4] -r 24 -an -c:v libx264 [outputfile.264]
			//
			stdoutfile = m_encode_dir + std::string("\\") + m_encode_base + mp_app->FormatStr("_%d_%s_report2.txt", m_id, export_timefrag.c_str() );
			//
			// build elementary stream filepath
			std::string elemOutfile = m_encode_dir + std::string("\\") + m_encode_base + mp_app->FormatStr("_%d_%s.264", m_id, export_timefrag.c_str() );
			//
			// build the ffmpeg args
			args = mp_app->FormatStr( "-i %s -r %1.2f -an -c:v libx264 %s > %s 2>&1", mp4outfile.c_str(), m_encode_fps, elemOutfile.c_str(), stdoutfile.c_str() );
			//
			// and finally:
			cmd = m_ffmpeg_path + " " + args;
			ret = system( cmd.c_str() );
			if (ret == -1)
			{
				ret = errno;
				mp_app->ReportLog( ReportLogOp::flush, mp_app->FormatStr( "FFmpeg error %d (during elementary stream encoding)\n", ret ) );
			}
		}

		// ffmpeg encoding complete, do we delete the files?
		if (m_post_encode == 0)
		{
			// YES!
			for (std::size_t i = 0; i < m_filepaths.size(); i++)
			{
				std::string& src = m_filepaths[i];

				if (remove( src.c_str() ) != 0)
				{
					m_final_delete_errors++;
				}
			}
		}
		else // no we move them to a final destination:
		{
			for (std::size_t i = 0; i < m_filepaths.size(); i++)
			{
				std::string& src = m_filepaths[i];

				std::string fname = mp_app->GetFilename( src );

				std::string dest = m_post_move_path + std::string("\\") + fname + std::string(".jpg");

				if (rename( src.c_str(), dest.c_str() ) != 0)
				{
					m_final_move_errors++;
				}
			}
		}

	} // end the first find_files

	mp_parent->SendVideoEncodeDoneEvent();
	m_threadExited = true;
}

/////////////////////////////////////////////////////////////////////////
void RenderCanvas::OnVideoEncodeDone(wxCommandEvent& event)
{
	if (!mp_app) 
		return;
	if (!mp_app->m_we_are_launched || m_terminating)
		return;

	// lock from other threads changing m_encoders
	std::unique_lock lock(m_encodersSharedMutex);	

	// loop over the FrameEncoders in the vector:
	std::vector<FrameEncoder*>::iterator it = m_encoders.begin();
	while (it != m_encoders.end())
	{
		bool erase_it(false);

		if ((*it)->m_threadExited)
		{
			if (!(*it)->m_threadJoined)
			{
				(*it)->mp_thread->join();
				(*it)->m_threadJoined = true;
				delete (*it)->mp_thread;
				(*it)->mp_thread = NULL;

				erase_it = true;
			}
		}

		if (erase_it)
		{
			it = m_encoders.erase(it);
		}
		else
		{
			it++;
		}
	}

	lock.unlock();
}

