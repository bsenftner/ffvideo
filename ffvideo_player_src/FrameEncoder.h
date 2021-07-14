#pragma once

#ifndef _FRAMEENCODER_H_
#define _FRAMEENCODER_H_

#include "ffvideo_player_app.h"


////////////////////////////////////////////////////////////////////////
class FrameEncoder
{ 
public:
	FrameEncoder(){};
	FrameEncoder( const FrameEncoder& fe){};

	// expects the (bottom) class members to be set before running as thread:
	void EncodeFrames( void );

	// for access to function that sends the VideoEncodeDone event: 
	RenderCanvas* mp_parent;

	// for access to app utility routines: 
	TheApp*	mp_app;

	// thread variables:
	std::thread*      mp_thread;
	std::atomic<bool> m_threadStarted;
	std::atomic<bool> m_threadExited;
	std::atomic<bool> m_threadJoined;

	// stats
	int32_t				m_id;				
	int32_t				m_move_errors;
	int32_t				m_final_move_errors;
	int32_t				m_final_delete_errors;

	// the file paths to the images being encoded:
	std::vector<std::string> m_filepaths;

	// these are the relevant fields from a VideoStreamConfig. If encode parameters changes, this needs updating:

	std::string		m_export_dir;						// where the exported frame jpegs are written
	std::string		m_export_base;					// basname of the exported jpegs
																				//
	int32_t				m_encode_interval;			// 0 = off, if > 0 is number of exported frames before encoding them
	int32_t				m_post_encode;					// an action code; 0 = delete frames, 1 = move to frame store
	std::string		m_ffmpeg_path;					// required path to ffmpeg.exe for encoding
	std::string		m_post_move_path;				// if m_post_encode = 1, is path to move image files
	std::string		m_encode_dir;						// directory encoded movies are saved, if encoding
	std::string		m_encode_base;					// encoding basename of movie
	float					m_encode_fps;						// 0 = last playback fps, else actual encoding fps 
	int32_t				m_encode_type;					// 0 = encode mp4s, 1 = encode mp4s and elementary streams
	int32_t				m_encode_width;					// set this or m_encode_height to -1 to retain aspect ratio of original frames			
	int32_t				m_encode_height;				// set this or m_encode_width to -1 to retain aspect ratio of original frames			
};





class windows_process
{
public:

	static PROCESS_INFORMATION launchProcess(std::string& app, std::string& arg, std::string& err)
	{
		// Prepare handles.
		STARTUPINFOW si;
		PROCESS_INFORMATION pi; // The function returns this
		ZeroMemory( &si, sizeof(si) );
		si.cb = sizeof(si);
		ZeroMemory( &pi, sizeof(pi) );

		err.clear();

		//Prepare CreateProcess args
		std::wstring app_w(app.length(), L' '); // Make room for characters
		std::copy(app.begin(), app.end(), app_w.begin()); // Copy string to wstring.

		std::wstring arg_w(arg.length(), L' '); // Make room for characters
		std::copy(arg.begin(), arg.end(), arg_w.begin()); // Copy string to wstring.

		std::wstring input = app_w + L" " + arg_w;
		wchar_t* arg_concat = const_cast<wchar_t*>( input.c_str() );
		const wchar_t* app_const = app_w.c_str();

		// Start the child process.
		if( !CreateProcessW(
			app_const,      // app path
			arg_concat,     // Command line (needs to include app path as first argument. args seperated by whitepace)
			NULL,           // Process handle not inheritable
			NULL,           // Thread handle not inheritable
			FALSE,          // Set handle inheritance to FALSE
			0,              // No creation flags
			NULL,           // Use parent's environment block
			NULL,           // Use parent's starting directory
			&si,            // Pointer to STARTUPINFO structure
			&pi )           // Pointer to PROCESS_INFORMATION structure
			)
		{
			err = "CreateProcess failed: " + std::to_string( GetLastError() );
		}

		// Return process handle
		return pi;
	}

	static bool checkIfProcessIsActive(PROCESS_INFORMATION pi, std::string& err)
	{
		err.clear();

		// Check if handle is closed
		if ( pi.hProcess == NULL )
		{
			err = "Process handle is closed or invalid: " + std::to_string( GetLastError() );
			return FALSE;
		}

		// If handle open, check if process is active
		DWORD lpExitCode = 0;
		if( GetExitCodeProcess(pi.hProcess, &lpExitCode) == 0)
		{
			err = "Cannot return exit code: " + std::to_string( GetLastError() );
			return false;
		}
		else
		{
			if (lpExitCode == STILL_ACTIVE)
			{
				return TRUE;
			}
			else
			{
				return FALSE;
			}
		}
	}

	static bool stopProcess( PROCESS_INFORMATION &pi, std::string& err)
	{
		err.clear();

		// Check if handle is invalid or has allready been closed
		if ( pi.hProcess == NULL )
		{
			err = "Process handle invalid. Possibly already closed: " + std::to_string( GetLastError() );
			return false;
		}

		// Terminate Process
		if( !TerminateProcess(pi.hProcess,1))
		{
			err = "ExitProcess failed: " + std::to_string( GetLastError() );
			return false;
		}

		// Wait until child process exits.
		if( WaitForSingleObject( pi.hProcess, INFINITE ) == WAIT_FAILED)
		{
			err = "Wait for exit process failed: " + std::to_string( GetLastError() );
			return false;
		}

		// Close process and thread handles.
		if( !CloseHandle( pi.hProcess ))
		{
			err = "Cannot close process handle: " + std::to_string( GetLastError() );
			return false;
		}
		else
		{
			pi.hProcess = NULL;
		}

		if( !CloseHandle( pi.hThread ))
		{
			err = "Cannot close thread handle: " + std::to_string( GetLastError() );
			return false;
		}
		else
		{
			pi.hProcess = NULL;
		}
		return true;
	}
};//class windows_process




#endif // _FRAMEENCODER_H_
