///////////////////////////////////////////////////////////////////////////////
// Name:        HelpWindow.cpp
// Author:      Blake Senftner
///////////////////////////////////////////////////////////////////////////////

#include "HelpWindow.h"

wxBEGIN_EVENT_TABLE(HelpWindow, wxFrame)
EVT_SIZE(HelpWindow::OnSize)
EVT_MOVE(HelpWindow::OnMove)
EVT_WEBVIEW_TITLE_CHANGED(wxID_ANY, HelpWindow::OnWebViewTitleChangedCB)
EVT_WEBVIEW_NAVIGATING(wxID_ANY, HelpWindow::OnWebViewNavigationCB)
wxEND_EVENT_TABLE()

#define HELP_WIN_WIDTH (885)
#define HELP_WIN_HEIGHT (700)


HelpWindow::HelpWindow(TheApp* app, wxWindow* parent )
	: wxFrame(parent, wxID_ANY, "FFVideo_player Help", wxDefaultPosition, 
	          wxSize(HELP_WIN_WIDTH,HELP_WIN_HEIGHT), wxRESIZE_BORDER | wxCAPTION | wxCLOSE_BOX | wxCLIP_CHILDREN), 
	  mp_app(app), mp_help_view(NULL)
{

	// embedded chrome web browser:
	mp_help_view = wxWebView::New(this, wxID_ANY,  wxWebViewDefaultURLStr, wxDefaultPosition, wxDefaultSize, wxWebViewBackendIE);

	// the memory: file system
	mp_help_view->RegisterHandler(wxSharedPtr<wxWebViewHandler>(new wxWebViewFSHandler("memory")));


	BuildFeatureBullets(); // only needs to run once

	m_include_A = false;	// set to true to demonstrate C++/JavaScript I/O
	m_include_B = false;
	m_include_C = false;
	//
	BuildHTMLBody();

	// maintained in a member variable, supports this code being used as a template: 
	m_html_filename = "FFVideoPlayerHelp.htm";

	// the html of the page written to this filename in memory:
	wxMemoryFSHandler::AddFile(m_html_filename, m_html_body );

	m_url = "memory:" + m_html_filename; // a memory file was created with this "url"


	mp_help_view->LoadURL( m_url );
	//
	wxFont ourFont = wxFont(11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_MEDIUM, false, _T("Arial"));
	//
	mp_help_view->SetFont(ourFont);
	mp_help_view->SetSize( 0, 0, HELP_WIN_WIDTH, HELP_WIN_HEIGHT );
  //
	wxPoint pos = parent->GetPosition();
	pos.x += 30;
	pos.y += 30;
	SetPosition( pos );


	Connect(wxEVT_CLOSE_WINDOW, (wxObjectEventFunction)&HelpWindow::OnCloseBox);

	// display our window:
	Show();
}

HelpWindow::~HelpWindow()
{
}

///////////////////////////////////////////////////////////////////
void HelpWindow::OnSize(wxSizeEvent& event)
{
	int32_t winWidth, winHeight;
	GetClientSize(&winWidth, &winHeight);

	if (mp_help_view)
	{
		mp_help_view->SetSize( 0, 0, winWidth, winHeight );
		RefreshWebView();
	}
}

///////////////////////////////////////////////////////////////////
void HelpWindow::OnMove(wxMoveEvent& event)
{
	if (!mp_app->m_we_are_launched)
		return;

	if (!IsShownOnScreen())
		return;
}


////////////////////////////////////////////////////////////////////////
void HelpWindow::OnCloseBox(wxCloseEvent& event)
{
	Hide();
	mp_app->mp_helpWindow = NULL;
	Destroy();
}


////////////////////////////////////////////////////////////////////////////////
void HelpWindow::BuildFeatureBullets( void ) 
{
	m_feature_bullet0 = 
		R"V0G0N(
	<div class="inset normal">
		Identified by <i>glowing numbers</i> in the images at left and below, each <B>Video Window</B> has these controls:</b><br>
		<span class="smaller">
			(If this documentation does not match the application version you have, please post an issue on the github's project.)
		</span>
	</div>
	<OL class="smaller">
		<LI>
			The <b>Leftmost Pull Down Menu</b> has these options:</br>
			<UL>
				<LI>
					<B>Pops</B> the <B>Video Stream Configuration Dialog</B> for the <B>Video Window</B>
				</LI/>
				<LI>
					<B>Creates</B> a new <B>Video Window</B>, or opens the next <B>Video Window</B> from the last saved multi-window layout
				</LI>
				<LI>
					<B>Restore</B> the previously saved <b>Multi-Window Layout</b> and their <b>Video Stream Configurations</b>
				</LI>
				<LI>
					<B>Save</B> the current <b>Multi-Window Layout</b> and their <b>Video Stream Configurations</b>
				</LI>
				<LI>
					<B>Play All Video Windows</B> not currently playing
				</LI>
				<LI>
					<B>Close All and Quit</B> ends playback of all windows, saves their last configurations, and quits the application
				</LI>
			</UL>
			The <B>Options</B> pull down menu has these options:</br>
			<UL>
				<LI>
					<B>Enable/Disable Face Detection</B> toggles the demo <i>video experiment</i>
				</LI>
				<LI>
					<B>Enable/Disable Face Feature Display</B> toggles a feature of the demo <i>video experiment</i>
				</LI>
				<LI>
					<B>Light/Dark Theme</B> switching, just changes the gradient on the <B>Video Window's</B> left side
				</LI>
				<LI>
					<B>Tile Video Windows</B> reconfigures the layout of the open <B>Video Windows</B> to tile, filling the
					available space of the monitor each video window is located 
				</LI>
				<LI>
					<B>Help...</B> opens the nifty <B>Help Window</B> you are now reading
				</LI>
				<LI>
					<B>About FFVideo Player...</B> opens a dialog describing who to blame for this software 
				</LI>
			</UL>
		</LI>
	</OL>
)V0G0N";
	m_feature_bullet1 = 
		R"V0G0N(
	<OL class="smaller">
		<LI value="2">
			The <B>Video Frame Display</B> from the currently active video stream. Video streams may be:
			<UL><LI><b>Media Files</b></LI><LI><b>USB Cameras</b></LI><li><b>IP Cameras</b> and <b>Streaming IP Video</b></LI></UL> 
			<B>Video Frames</B> scale to fit the available window area, right justifying to provide visible space 
			for the <B>Video Overlay Text</B>. </br>
			<B>Video</B> plays as fast as possible, preserving maximum CPU for whatever
			video experiment, video analysis, or deep learning process that is running. </br>
			For example, visible in the above video frame is optional <b>facial feature identification and tracking</b>, a demo
			<i>video frame experiment</i>.</br>
			The under-the-hood <b>FFmpeg-based video library</B> used by this application has been <B>custom modified</B> to 
			handle <B><i>unexpected video stream dropouts</i></B>, an event the unmodified FFmpeg libraries do not handle. 
		</LI>
		<LI value="3">
			In the center of the video frame are <B>video playback controls:</B>
			transparent overlay buttons for <B>Play</B>, <B>Pause</B>, <B>Step-Back</B>, <B>Stop</B>, and <B>Step-Forward</B></br> 
			These <B>playback controls</B> become visble when the mouse is near the center of the video window.</br>
			If the video stream does not support pausing, meaning <B>USB Cameras</B> and <B>IP Video</B>, the <B>Pause</B> button
			is the same as <B>Stop</B> and <i>Frame Scrbbing While Paused</i> is unavailable.</br>
			In addition to these visible controls, the video window also responds to the following <B>keyboard Commands:</B>
			<UL>
				<LI>
					<B>Right Arrow:</B> if a <b>Media file</b>, this triggers a <B>pause</B> if <B>playback</B> is active, 
					and if already paused a <B>10-second backwards skip</B> occurs. If not a <b>Media file</b> the key press is ignored.
				</LI/>
				<LI>
					<B>Left Arrow:</B> if a <b>Media file</b>, this triggers a <B>pause</B> if <B>playback</B> is active, 
					and if already paused a <B>10-second forwards skip</B> occurs. If not a <b>Media file</b> the key press is ignored.
				</LI/>
				<LI>
					<B>Down Arrow:</B> if a <b>Media file</b>, this triggers a <B>pause</B> if <B>playback</B> is active, 
					and if already paused a <B>Single Frame Step Backward</B> occurs. If not a <b>Media file</b> the key press is ignored.
				</LI/>
				<LI>
					<B>Up Arrow:</B> if a <b>Media file</b>, this triggers a <B>pause</B> if <B>playback</B> is active, 
					and if already paused a <B>Single Frame Step Forward</B> occurs. If not a <b>Media file</b> the key press is ignored.
				</LI/>
				<LI>
					<B>S:</B> this <B>stops</B> active playback and <B>clears the frame stepping scrub buffer.</B> 
					If playback is not active the key press is ignored.
				</LI/>
				<LI>
					<B>Spacebar:</B> this issues a <B>Play</B> request to the underlying video library 
					If playback is already active the key press is ignored.
				</LI/>
			</UL>
		</LI>
		<LI value="4">
			Initially at the top left of the <b>Video Window</b> is video overlay information describing the current setup for the <b>Video Window</b>.
			The font used to display this information can be changed via the <B>Stream Configuration Dialog</B>.
			This information can be moved by clicking and dragging the text. 
			(At the moment, dragging the text over the video playback controls activates them - a known bug that needs to be fixed.) 
		</LI>
		<LI value="5">
			At the bottom of a <b>Video Window</b> is a <b>scrub bar</b>, only active for <b>Video files</b>, and the <b>status bar</b> is beneath.
			On the left side of the <b>status bar</b> is information about the window and stream combination, and on the right
			is key information about the current stream, such as the <B>Video File's</B> path, <B>USB PIN</B> or the <B>Video URL</B>. 
		</LI>
	</OL>
)V0G0N";
	m_feature_bullet2 = 
		R"V0G0N(
		<div class="smaller" style="margin-top: 20px; margin-bottom: 0px;">
		Above, at left, is the <b>Video Stream Configuration Dialog</b>, and to the right is the <b>USB Camera Configuration Dialog</b> and 
		beneath that the <b>Font Configuration Dialog</b>. When the <b>Stream Type</b> is <b>USB</b> the <b>USB Camera Configuration Dialog</b> can
		be displayed with a button click. Likewise, the when the <b>Video Overlay Font's <i>Change</i> button</b> is pressed its config dialog pops. 
		The top of the <b>Video Stream Configuration Dialog</b> holds	the essential controls, and the lower 2/3rds are optional 
		<b>frame exporting and encoding</b> into <b>.MP4 files</b> for use in conventional media
		players such as <b>Windows Media Player</b>, or <b>VLC</b>. Secondary encoding to <b>H.264 Elementary Streams</b> can also be used for easy re-streaming.  
		</div>
	<OL class="smaller">
		<LI value="1">
					<b>Video Stream Name:</b> is a text field for a <i>stream name</i>, this data is not used for anything beyond saving stream configuration settings 
					under this name.</br>
					<b>Note:</b> on the far right of the <b>Video Stream Name</b> text entry field is a <i>down icon</i>, clicking that icon drops a menu for selection
					between all the <b>Video Stream Configurations</b> used by this installation of the application. Selecting a <b>Stream Name</b> other than the original
					<b>Stream Name</b> causes the selected stream's configuration to be loaded and become the active stream for this <b>Video Window</b> 
					(if the 'Ok' button is then clicked.) 
		</LI>
		<LI value="2">
					<b>Video Stream Type:</b> a pull-down selection with these options: 
			<UL>
				<LI>
					<b>Video File:</b> the video stream can be loaded via a local connected storage device, such as a hard drive, network attached drive, a drive on a local
					network attached computer, and so on. 
					Video files are unique because they can achieve very high frame rates, can be seeked, scrubbed, and looped. Other stream types can only be played.<br>
					When the left-hand selector is <b>Video File</b> the middle button becomes a <b>File Navigator</b> button so a <b>Video File</b> can be located and
					selected for playback. If the  <b>Loop Media Files</b> checkbox is checked, the stream auto-loops. When auto-looping, if frame exporting is active it
					turns off upon re-loop. 
				</LI/>
				<LI>
					<b>USB Camera:</b> the video stream is from a USB Camera directly attached to this computer. <b>USB Cameras</b> are somewhat unique from the perspective
					they can provide a varying frame rate, depending upon how much light is available to the camera sensor. They also provide a format and resolution 
					configuration dialog, which the other stream types do not provide. <br>
					<b>USB Cameras</b> can not be paused or scrubbed, so those features are unavailable when a <b>USB Camera</b> stream.</br>
					When the left-hand selector is <b>USB Camera</b> the middle button becomes a <b>Webcam Config</b> button so the <b>USB Camera</b> can be selected, if
					multiple are attached to the computer, and the selected webcam's format and resolution chosen. Additionally, the <b>Loop Media Files</b> checkbox will
					do nothing when the stream type is <b>USB Camera</b>.
				</LI/>
				<LI>
					<b>IP Camera/Stream:</b> the video stream is from a network, such from a security camera on the local WiFi or Ethernet LAN, or an Internet video service
					such as YouTube. All that is necessary is the playback URL of the stream. <br>
					<b>IP Video</b> can not typically be paused or scrubbed, so those features are unavailable when playing an <b>IP Camera/Stream</b> stream.</br>
					When the left-hand selector is <b>IP Camera/Stream</b> the middle button becomes a <b>Previous URLs</b> button that pops a <b>Previous URLs Dialog</b> 
					with the history of all previously played URLs for selection and playback. The <b>Loop Media Files</b> checkbox will
					do nothing when the stream type is <b>IP Camera/Stream</b>.
				</LI/>
			</UL>
		</LI>
)V0G0N";
	m_feature_bullet2 += 
		R"V0G0N(
		<LI value="3">
					<b>Path / USB Pin / URL:</b> is a text entry field that is auto-filled by operating the <b>Video Stream Type</b> controls. For URLs, see below:
					</br></br>
					In the case of <b>IP Cameras</b> or
					video from <b>IP Streaming Services</b> not played before, the playback <b>URL</b> should be entered directly into this field. 
					If the playback of the <b>URL</b> is successful, it will be added to the application history. If the <b>URL</b> has been played before, 
					it is in the application history (if the play was successful,) and the righthand down-icon can be clicked to generate a pull down menu of 
					previous playback <b>URLs</b>.
					</br></br>
					In the case of <b><i>unknown camera URLs</i></b>, beyond reading your camera manuals and Internet research, it is suggested one use <b>ONVIF Compitable IP Cameras</b>
					and run a copy of <b><a href="https://sourceforge.net/projects/onvifdm/">The ONVIF Device Manager</a></b> for easy playback <b>URL</b> discovery.
					</br></br>
					In the case of <b><i>Internet Video Services</i></b>, if an <b><i>embed URL</i></b> is provided this may be successful. However, many <b>Internet Video Services</b> 
					do not provide direct playback <b>URLs</b>. This can be worked around by first playing the stream with <b>VLC</b> via their <b>Open Network Stream dialog</b>;  
					<b>VLC</b> performs the necessary server negotiating, and the <i>direct playback URL</i> can be retrieved from <b>VLC's Tools -> Media Information Dialog</b> 
					at the bottom in the field labeled <b>location</b>. Sometimes this <b>URL</b> is very long and I suspect it is overrunning an internal buffer within <b>FFmpeg</b> 
					because sometime after a very long, longer than 2K characters, <b>URL</b> is used the appliction is unstable. 
		</LI>
		<LI value="4"> 
					<b>Frame Display Interval:</b> is an integer text entry field with spinner buttons for easy value adjustments. This is the frequency decompressed frames are
					delivered to the application for display. If set to 1, every frame is delivered for display, if set to 2 every other frame is sent for display. This may be
					set as high as 10. When set to 0, the <b>Auto Frame Interval</b> logic is enabled.
					</br></br>
					<b>Auto Frame Interval</b> is pre-configured settings for the <b>Frame Interval</b> based upon frame horizontal resolution. Sometimes an <b>IP Camera</b> will
					reset and a pre-configured stream's resolution will change to an unexpected setting. Likewise, <b>IP Video Services</b> deliver a resolution
					calculated dynamically by time of day and location of request, regardless of what resolution may be requested. For these situations, <b>Auto Frame Interval</b>
					may be used to provide a series of resolutions and frame intervals to be used. clicking the <b>Auto Frame Interval Profile</b> button pops a dialog with an
					explainer how to enter a custom <b>Auto Frame Interval Profile</b>. 
		</LI>
		<LI value=5"> 
					<b>Optional Frame Filter:</b> is a text entry field for a <b><a href="https://ffmpeg.org/ffmpeg-filters.html#Video-Filters">AVFilterGraph Video Filter</a></b>.
					An <b>AVFilterGraph Video Filter</b> is plain text containing one or more <b>AVFilter Graph Commands</b>. There are many video filters, each with its own unique and
					indipendant parameters and syntax. However, separate video filters may be <i>chained</i>, the output of one becomming the input for another. Chaining of video filters
					is accomplished by placing different video filter commands one after another with a single semicolon: ";" between them. Video filters are applied
					to the decompressed frames by <b>FFmpeg</b> before they are delivered to the appliction's <b>Frame Display or Export Handlers</b>. 
					Not all <b>AVFilterGraph Video Filters</b> will work - filters requiring multiple input sources, multiple outputs, and some filters that apply filtering
					across multiple frames do not work in this application. 
					Examples of filters that will work include:
					<ul>
					<li><b>Disable Frame Filtering</b> (if active): none</li> 
					<li>A <b>Sharpen Convolution</b> filter: convolution="0 -1 0 -1 5 -1 0 -1 0:0 -1 0 -1 5 -1 0 -1 0:0 -1 0 -1 5 -1 0 -1 0:0 -1 0 -1 5 -1 0 -1 0"</li> 
					<li>An <b>Edge Enhance Convolution</b> filter: convolution="0 0 0 -1 1 0 0 0 0:0 0 0 -1 1 0 0 0 0:0 0 0 -1 1 0 0 0 0:0 0 0 -1 1 0 0 0 0:5:1:1:1:0:128:128:128"</li> 
					<li>A <b>Vintage Footage</b> filter: curves=r='0/0.11 .42/.51 1/0.95':g='0/0 0.50/0.48 1/1':b='0/0.22 .49/.44 1/0.8'</li> 
					<li>A <b>Compression Artifact Denoising</b> filter: dctdnoiz=4.5</li> 
					<li>A <b>Deblocking</b> filter for removal of compression artifacts: deblock=filter=weak:block=4</li> 
					<li>A <b>2nd Deblocking</b> filter: deblock=filter=strong:block=4:alpha=0.12:beta=0.07:gamma=0.06:delta=0.05</li> 
					<li>A <b>Grid Rendering</b> filter: drawgrid=width=100:height=100:thickness=2:color=red@0.5</li> 
					<li>An <b>Edge Detection</b> filter: edgedetect=low=0.1:high=0.4</li> 
					<li>An <b>Edge Detection Painterly Effect</b> filter: edgedetect=mode=colormix:high=0</li> 
					<li>A <b>High-pass</b> filter: fftfilt=dc_Y=128:weight_Y='squish(1-(Y+X)/100)'</li>
					<li>A <b>Sharpen</b> filter: fftfilt=dc_Y=0:weight_Y='1+squish(1-(Y+X)/100)'</li>
					<li>A <b>Blur</b> filter: fftfilt=dc_Y=0:weight_Y='exp(-4 * ((Y+X)/(W+H)))</li>
					<li>A <b>Pixel Math</b> filter, modify RGB components depending on pixel position: geq=r='X/W*r(X,Y)':g='(1-X/W)*g(X,Y)':b='(H-Y)/H*b(X,Y)'</li>
					<li>An <b>Emboss</b> filter: format=gray,geq=lum_expr='(p(X,Y)+(256-p(X-4,Y-4)))/2'</li>
					<li>A <b>Pixel Color Equasion</b> filter, this sets the Hue to 90 degrees and the Saturation to 1.0: hue=H=PI/2:s=1</li>
					<li>A <b>Frame Rotation</b> filter: rotate=PI/6</li>
					<li>A <b>Vignette</b> filter: vignette=PI/4</li>
					<li>A <b>Flickering Vignette</b> filter: vignette='PI/4+random(1)*PI/50':eval=frame</li>
					<li>A <b>Color Mapping</b> filter, map the darkest pixels to red, the brightest to cyan: normalize=blackpt=red:whitept=cyan</li>
					<li>A <b>Sharpen</b> filter: convolution="0 -1 0 -1 5 -1 0 -1 0:0 -1 0 -1 5 -1 0 -1 0:0 -1 0 -1 5 -1 0 -1 0:0 -1 0 -1 5 -1 0 -1 0"</li>
					<li>A <b>Sepia Tone</b> filter: colorchannelmixer=.393:.769:.189:0:.349:.686:.168:0:.272:.534:.131</li>
					<li>A <b>Motion</b> filter, only Displays Moving Pixels: tblend=all_mode=grainextract</li>
					<li>An <b>RGB Channel Separation</b> filter, growing resolution 4x: split=4[a][b][c][d];[b]lutrgb=g=0:b=0[x];[c]lutrgb=r=0:b=0[y];[d]lutrgb=r=0:g=0[z];[a][x][y][z]hstack=4</li>
					<li>A <b>Vectorscope and Waveforms</b> filter, it  grows resolution too: format=yuv444p,split=4[a][b][c][d],[a]waveform[aa],[b][aa]vstack[V],[c]waveform=m=0[cc],[d]vectorscope=color4[dd],[cc][dd]vstack[V2],[V][V2]hstack</li>
					<li>A <b>Histogram and Waveforms</b> filter, also grows resolution: format=gbrp,split=4[a][b][c][d],[d]histogram=display_mode=0:level_height=244[dd],[a]waveform=m=1:d=0:r=0:c=7[aa],[b]waveform=m=0:d=0:r=0:c=7[bb],[c][aa]vstack[V],[bb][dd]vstack[V2],[V][V2]hstack</li>
					</ul>
		</LI>
)V0G0N";
	m_feature_bullet2 += 
	R"V0G0N(
		<LI value="6"> 
					<b>Frame Export Interval:</b> is an integer text entry field with spinner buttons for easy value adjustments. This is the frequency decompressed frames are
					written to disk at the location specified with the <b>Frame Export Directory</b> button. This is completely independant of the <b>Frame Display Interval</b>. 
					If <b>Frame Exporting</b> is set to 1, every frame decompressed is exported to disk, if set to 2 every other frame decompressed is written to disk. 
					Files are written in JPEG format. The <b>Export Basename</b> button is used to specify a filename
					prefix for the exported image files. Beneath this line of controls is an example of the filepath and filename that will be used. 
					</br></br>
					Note that frames are displayed ASAP, and passed to a background process for writing to disk. The ability to keep up, writing frames to disk, is up to your
					computer and speed of the disks. Using an SSD drive helps, but be aware of multiple videos contending to write at the same time if playing more than one
					video at a time has <b>Frame Exports</b> enabled. If the computer is unable to keep up, unwritten but scaled for output frames accumulate in memory; so having
					an appropriate amount of RAM is a good idea if your disk(s) can not keep up. 
		</LI>
		<LI value="7"> 
					<b>Example Export File:</b> is the file path and basename for frame exports as configured by the <b>Frame Export Directory</b> and <b>Export Basename</b> buttons.
					The timecode is an ISO Extended Timecode with milliseconds, corresponding to the time the video frame is added to the frame export queue, which is also the same
					time the frame would be displayed, if it is being displayed. 
		</LI>
		<LI value="8"> 
					<b>Frame Export Scale:</b> is a text entry field for a <i><b>normalized floating point value</b></i>, meaning a value between 0.0 and 1.0. The value is treated like
					a percentage, defining a uniform frame reduction in both width and height. If one is working with slow disks, this can help a lower powered system maintain real time.  
		</LI>
		<LI value="9"> 
					<b>Frame Export Quality:</b> is an integer text entry field with spinner buttons and a range between 0 and 100. The value is the JPEG compression quality used when saving.  
		</LI>
		<LI value="10"> 
					<b>Encoding Interval:</b> is an integer text entry field with spinner buttons. This is the frequency <b>Exported Frames</b> are compressed back into video streams,
					potentially with alterations due to the use of <b>AVFilterGraph Video Filters</b> or simply for archive, because the streams originate from security cameras. 
		</LI>
		<LI value="11"> 
					<b>Encoding Interval:</b> is an integer text entry field with spinner buttons. This is the frequency <b>Exported Frames</b> are compressed back into video streams,
					potentially with alterations due to the use of <b>AVFilterGraph Video Filters</b> or simply for archive, because the streams originate from security cameras. The
          <b>Encoding Directory</b> and <b>Encoding Basename</b> buttons are used to specify the directory the new encoded video stream files are saved, and the filename
					prefix to use when saving them. 
		</LI>
		<LI value="12"> 
					<b>Example Encode File:</b> is the file path and basename for new encoded stream files as configured by the <b>Encoding Directory</b> and <b>Encoding Basename</b> 
					buttons. The <b>N</b> in the filename corresponds to the Nth encoding since play began on that stream. 
		</LI>
		<LI value="13"> 
					<b>FFmpeg.exe Path:</b> is the path to the <b>FFmpeg</b> executable to use when compressing new streams. At minimum, the <b>FFmpeg</b> executable must be compiled 
					with the <b>H.264 Codec</b> for steam encoding to work. Note that a pre-compiled version is available from the author's Github project for this application.  
					</br></br>
					To the right of the <b>Set path to FFmpeg.exe</b> button is a pull-down selector for choosing what happens to the <b>Exported Video Frames</b> after encoding.
					<b>They can be deleted,</b> or </b>copied to another location.</b> If the <b>Exported Video Frames</b> are moved to another location, the button to the right 
					labeled <b>Post Encode Move Path</b> is used to set where image files are coped after encoding. 
					</br></br>
					Beneath this line of controls is text showing the active <b>FFmpeg.exe</b> path, and beneath that is more text labeled <b>Post Encode Move</b>, that is the path
					configured with the <b>Post Encode Move Path</b> button, it is the final destination of <b>Exported Video Frames</b> if they are not deleted. 
		</LI>
		<LI value="14"> 
					<b>Encoding FPS:</b> is a text entry field for the <b>playback frames per second rate</b> of the encoded video streams. This may contain integer or floating point
					values. Although this application ignores a video stream's FPS setting, conventional video players do not and this setting will be respected by them. 
		</LI>
		<LI value="15"> 
					<b>Encoding Type:</b> is pull-down selection for choosing the video encoding. Current options are:
					<ul>
						<li>
							<b>Encode standard media player H.264 MP4 files:</b> this encodes .MP4 files using an <b>FFmpeg</b> command line of this format:
							<div class="inset">
									ffmpeg.exe -f image2 -safe 0 -f concat -i list.txt -vf scale=[userval]:[userval] -r [userval] -c:v libx264 -pix_fmt yuv420p -refs 16 -crf 18 -preset ultrafast output3.mp4
							</div>
							Videos encoded with these settings retain decent quality, play in consumer video players, and upload to video services such as YouTube fine. 
						</li>
						<li>
							<b>Encode H.264 MP4 and H.264 Elementry Stream files</b> this encodes twice, first to an H.264 MP4 file, and then the MP4 is converted into its Elementary Stream using:
							<div class="inset">
									ffmpeg -i inputfile.mp4 -r [userval] -an -c:v libx264 outputfile.264
							</div>
							H.264 Elementary Streams may be placed into a directory running the <b><a href="http://live555.com/mediaServer/">Live555 Media Server</a></b> and immediately re-streamed virtually
							anywhere. (The author of this application is a huge fan of the Live555 libraries, and a future version of this software will support deeper integration with the Live555 libraries.)  
							With a little creative hacking, one could create a video feedback loop with evolutionary video filtering experiments, or place cameras around to create a video security system
							with experiment capabilities. </i>(That <b>Motion Video Filter</b> sure looks interesting...)</i> 
						</li>
					</ul>
		</LI>
		<LI value="16"> 
					<b>Encoding Width, Height:</b> is a text entry field for the <b>width, height</b> of the final video MP4 and/or .264 stream(s). This is a comma separated pair of integer
					values, with a special value of "-1" recognized to one of:
					<ul>
						<li><b>-1,-1:</b> both width and height are -1, use same width, height as the source</li>
						<li><b>[userval],-1:</b> width is an integer and height is -1, output a video that is width wide, and the height is scaled to be aspect-ratio correct</li>
						<li><b>-1,[userval]:</b> width is -1 and height is an integer, output a video that is height tall, and the width is scaled to be aspect-ratio correct</li>
					</ul>
		</LI>
		<LI value="17"> 
					<b>Video Overlay Font:</b> is a button that pops the <b>Font Configuration Dialog</b> for that <b>Video Window</b>. Any fonts your computer system has installed will be available. 
		</LI>
	</OL>
)V0G0N";
}

////////////////////////////////////////////////////////////////////////////////
void HelpWindow::BuildHTMLBody( void ) 
{ 
	// the html of the page in a string:
	m_html_body = "<html><body>" + GetWebContentCSS();
	m_html_body += GetWebContentJavaScriptCallbacks();
	m_html_body += R"V0G0N(
<div style="margin-bottom: 10px;">
<B>FFVideo Player</B> is a free and open source application, it is a no-audio video player intended 
   for video experiments and developers learning how to code media applications:<br>
<span class="smaller">(enlarge window for a better view)</span>
</div>
)V0G0N";

	int32_t winWidth, winHeight;
	GetClientSize(&winWidth, &winHeight);

	wxString im_path0 = mp_app->m_data_dir + "\\gui\\FFVideo00.png";
	wxString im_tag0 = wxString::Format( "<img width=\"%d\" src=\"%s\">", winWidth-40, im_path0.c_str() );
	im_tag0 += R"V0G0N(
		<div class="inset2 smaller">
		Note: copywritten images are legally used via being technology demonstrations of this educational application.
		</div>
)V0G0N";

	// style=\"margin-bottom: -200px;\" 

	wxString im_path1 = mp_app->m_data_dir + "\\gui\\FFVideo01frag.png";
	wxString im_tag1a = wxString::Format( "<img src=\"%s\">", im_path1.c_str() );

	wxString im_path1b = mp_app->m_data_dir + "\\gui\\FFVideo01.png";
	wxString im_tag1b = wxString::Format( "<img width=\"%d\" src=\"%s\">", winWidth-40, im_path1b.c_str() );
	im_tag1b += R"V0G0N(
		<div class="inset2 smaller">
		Note: copywritten images are legally used via being technology demonstrations of this educational application.
		</div>
)V0G0N";

	wxString im_path2 = mp_app->m_data_dir + "\\gui\\FFVideo02.png";
	wxString im_tag2 = wxString::Format( "<img width=\"%d\" src=\"%s\">", winWidth-40, im_path2.c_str() );

	m_html_body += im_tag0;
	m_html_body += R"V0G0N(
<div class="normal" style="margin-top: 20px; margin-bottom: 20px;">
<B>FFVideo</B> supports multiple simultanious <i>minimum delay playback</i> video windows, seek, 
   scrubbing, basic face detection, plus the surrounding code necessary for a moderately professional application, 
	 such as persistance for end-user settings, and this embedded web browser providing a 'help window'.<br><br>
	 The idea of this application is to provide a basic video app framework
	 for people wanting to learn, and experiment with video filters without the overhead of audio processing or 
	 the normal frame delays of ordinary video playback. If one is training models with video data, and want to
	 preserve the maximum amount of resources for training, this provides a nice framework for doing so.<br><br>
	 All source code is at the author's repository on Github.com.<br><br>
	 Please use the following images and text to guide your use of the application GUI:
</div>
)V0G0N";
	m_html_body += "<div class=\"normal floatLeft\">";
	m_html_body += im_tag1a;
	m_html_body += m_feature_bullet0;
	m_html_body += "</div>";
	m_html_body += im_tag1b;
	m_html_body += "<div class=\"normal\">";
	m_html_body += m_feature_bullet1;
	m_html_body += "</div>";
	m_html_body += im_tag2;
	m_html_body += "<div class=\"normal\">";
	m_html_body += m_feature_bullet2;
	m_html_body += "</div>";


	m_html_body += "<br>";
	m_html_body += "<span>";
	//
	if (m_include_A)
		m_html_body += "<a id=\"del1\" href=\"javascript:Delete(1);\">Delete A</a>&nbsp;&nbsp;";
	//
	if (m_include_B)
		m_html_body += "<a id=\"del2\" href=\"javascript:Delete(2);\">Delete B</a>&nbsp;&nbsp;";
	//
	if (m_include_C)
		m_html_body += "<a id=\"del3\" href=\"javascript:Delete(3);\">Delete C</a>";
	//
	m_html_body += "</span>";
	/* */

	m_html_body += "</body></html>";
}

// --------------------------------------------------------------------
wxString HelpWindow::GetWebContentCSS(void)
{
	wxString data = R"V0G0N(
<style>
body {
 font-family: Arial, Helvetica, sans-serif;
 font-size: 1.25em;
 line-height: 1.5;
 color: white;
 background-color: #0F84BE;
}
a, a:hover, a:active, a:visited { color: white; }
ol {
	list-style-type: decimal;
	list-style-position: outside;
}
li {
	padding: 10px;
}
.inset {
	padding-left: 20px;
}
.inset2 {
	padding-left: 50px;
}
.normal {
	font-size: 0.9em;
}
.normal img {
	float: left;
	margin-right: 40px;
}
.floatLeft {
	float: left;
}
.smaller {
	font-size: 0.75em;
}
</style>
)V0G0N";

	return data;
} 



// --------------------------------------------------------------------
wxString HelpWindow::GetWebContentJavaScriptCallbacks(void)
{
	wxString data = R"V0G0N(
<script type="text/javascript">
	function Delete(i)
	{
	  if (confirm("Really?"))
	  {
	    document.title='CALLBACK' + i;
	  }
	  else
	  {
	    document.title='CALLBACK0';
	  }
	};
</script>
)V0G0N";

	return data;
}

////////////////////////////////////////////////////////////////////////////////
void HelpWindow::ReplaceHTML( wxString& body ) 
{ 
	wxMemoryFSHandler::RemoveFile(m_html_filename);
	wxMemoryFSHandler::AddFile(m_html_filename, body);
}

////////////////////////////////////////////////////////////////////////////////
void HelpWindow::RefreshWebView( void ) 
{ 
	BuildHTMLBody();
	ReplaceHTML( m_html_body );

	mp_help_view->Reload();
}

////////////////////////////////////////////////////////////////////////////////
void HelpWindow::OnWebViewTitleChangedCB(wxWebViewEvent& event)
{
	wxString msg( event.GetString() );

	wxString prefix("CALLBACK"), rest;
	if (msg.StartsWith( prefix, &rest ))
	{
		wxString bsjnk( "Callback " );
		bsjnk += rest;
		wxMessageBox( bsjnk );

		// get "id" of element clicked:
		int element_id = wxAtoi(rest);

		if (element_id==1) m_include_A = false;
		else if (element_id==2) m_include_B = false;
		else if (element_id==3) m_include_C = false;

		RefreshWebView(); 
	}
}

////////////////////////////////////////////////////////////////////////////////
void HelpWindow::OnWebViewNavigationCB(wxWebViewEvent& event)
{
	/*
	wxString msg( "Navigation request to : " );
	msg += event.GetURL();
	wxMessageBox( msg ); 
	*/
}