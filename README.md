# FFVideo
##### <i>Last update July 31, 2021</i></br>
An example FFmpeg lib, and wxWidgets Player application with video filters and face detection, it is a no-audio video player intended for video experiments and developers 
learning how to code media applications. 

![FFVideo00](https://user-images.githubusercontent.com/1216815/125710205-65eaf07c-31b3-43e0-b660-867780cbaba5.png)

FFVideo Player supports multiple simultanious minimum delay playback video windows, seek, scrubbing, basic face detection, plus the surrounding code necessary for a moderately
professional application, such as persistance for end-user settings, and an embedded web browser providing a 'help window'.

The idea of this application is to provide a basic video app framework for developers wanting to learn, and experiment with video filters without the overhead of audio processing
or the normal frame delays of ordinary video playback. If one is training models with video data, and want to preserve the maximum amount of resources for training, this provides
a nice framework for doing so. This is a C++ Visual Studio 2019 IDE Solution and Project. 

![Interfaces](https://user-images.githubusercontent.com/1216815/125846001-2d363d08-35ac-4ff0-9538-bbf7fa7f1b7f.png)

Some of the coding examples in this project include:
 - An FFmpeg library supporting 
   - video files, USB Cameras, IP Cameras and IP video services
   - video file seeks, scrubbing, 
   - unlimited, chained single source AVFilterGraph filters, 
   - frame exporting
   - This project uses the author's modified FFmpeg located here 
     - https://github.com/bsenftner/FFmpeg
     - The modification is to support unexpected IP and USB stream dropouts
   - This project also uses the author's SQLite3 wrapper library, located here:
     - https://github.com/bsenftner/kvs
   - And this project uses Jorge L Rodriguez's stb image scaling header only library:
     - http://github.com/nothings/stb 
 - A multi-threaded wxWidgets Video Player application
   - Multiple simultanious video windows
   - Exported video frames collected and re-encoded as H.264 .MP4 and .264 Elementary Streams 
   - Easy access to AVFilterGraph Video Filters and video experimentation
   - Integration with Dlib and a basic example of Face Detection and of Face Landmark Recovery
   - An embedded web browser as the "Help" window
   - Lots in-code of documentation describing how, what and why 

When playing an HD film trailer from a local SSD drive, frame rates as high as ~~700~~ 70 fps (after reverting fo FFmpeg 4.2.3) can be achieved while 
only using 2 cores of a Ryzen 7. Extended time tests show no memory leakage or stale/dead thread acculumation. 

Although vcpkg is used, integration issues led to independant building of Boost, FFmpeg, GLEW, and WxWidgets. 
For these reasons the following environment variables are used within the project's Visual Studio 2019 solution and VS projects to locate these libraries:
 - **BoostRoot**    Set to root of the Boost 1_76_0 directory hierarchy
 - **FFmpegRoot**   Set to the FFmpeg installation directory root, author used version ~~4.4~~ 4.2.3 - new build with FFmpeg 4.2.3 is more stable, see below
 - **FFmpegDebugRoot** Set to the FFmpeg debug build directory root
 - **FFvideoRoot**  Set to the github root of this project
 - **GLEWRoot**     Set to root of GLEW 2.2.0
 - **kvsRoot**      Set to the github root of https://github.com/bsenftner/kvs
 - **vcpkgRoot**    Set to the installation root of vcpkg
 - **WXWIN**        Set to the installation root of wxWidgets, author used version 3.1.3 with the optional wxWebView component built and required for this project

Through vcpkg integration with Visual Studio, the TurboJPEG, jpeg, png, zlib, and Dlib libraries are also used.
Vcpkg also failed to integrate with Visual Studio 2019 out of the box, so a custom *triplet file* named **x64-windows-static-142.cmake** was placed inside
the $(vcpkgRoot)\triplets directory with this contents:
```
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_PLATFORM_TOOLSET v142)
```
Once that custom vcpkg triplet was in place, vcpkg correctly generates Visual Studio 2019 x64 static builds using the Windows 142 toolset

Through wxWidgets OpenGL, and an embedded web browser is encorporated. The embedded web browser requires a custom build of wxWidgets. 

Dlib's face detection model file shape_predictor_68_face_landmarks.dat is also required for this project, it can be downloaded in compressed form from:

   http://dlib.net/files/shape_predictor_68_face_landmarks.dat.bz2
   
After decompression, it should be placed in the project's bin directory, just beneath the project's git root

New: 
Added a menu option for display of detected faces. Beta, only displays first detected face per frame at the moment.

<img src="https://user-images.githubusercontent.com/1216815/126713258-f1d23ba1-3fce-4038-8cde-e1567e365362.png" width=400>

New July 28, 2021:
Modified the display of detected faces to optionally <i>standardize the collected face images</i> to be presented as close as possible to <b>Standard Passport Format</b>, with the eyes rotated to be level and re-evaluated to a 300x300 pixel image, regardless of source dimensions. 

demonstrating tilted head registration | demonstrating multiple heads
-------------------------------------- | ----------------------------
<img src="https://user-images.githubusercontent.com/1216815/127404166-f7139c59-a60c-4078-9ce5-cbe8a576f8aa.jpg"> | <img src="https://user-images.githubusercontent.com/1216815/127430640-42680645-683b-4cd0-9cea-cda0a0059b26.png">

New July 29, 2021:</br>

Switched to using the <a href="http://ermig1979.github.io/Simd/index.html">SIMD Library</a> for RGBA to RGB and to grayscale conversions. Multiple face detection code changes, such as adding a <i>precision</i> control and switching to doing face detections in grayscale.</br>

<img src="https://user-images.githubusercontent.com/1216815/127581406-9aadb7a9-f2ff-40df-9363-239497f2df72.png" width=40% >

New July 31, 2021:</br>

Experimenting with the 81 point face landmark model from https://github.com/codeniko/shape_predictor_81_face_landmarks </br>
In RenderCanvas.cpp, the bottom of the RenderCanvas() constructor is a call to</br> 
m_faceDetectMgr.SetFaceModel( FACE_MODEL::eightyone ); </br>
just change that to m_faceDetectMgr.SetFaceModel( FACE_MODEL::sixtyeight ); to revert back to the original face landmarks. </br>
Note there is some glue logic when getting the "face chips" for the 81 point face model to work.

![detectedFaceDisplay05](https://user-images.githubusercontent.com/1216815/127749077-e9e5939e-1a73-422b-9910-76bddd899cf9.png)


Known issues:

(Rebuilding against FFmpeg 4.2.3 seems to have removed the replay instabilities, but it's not as fast anymore. See note, mid-readme)

PlayAll has been modified to play USB cameras first, one by one, and then the other stream types are played simultaniously. This seems to sidestep USB stream startups being non-thread safe. 
