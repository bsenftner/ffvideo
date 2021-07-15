# FFVideo
An example FFmpeg lib, and wxWidgets Player application with video filters and face detection, it is a no-audio video player intended for video experiments and developers 
learning how to code media applications.

![FFVideo00](https://user-images.githubusercontent.com/1216815/125710205-65eaf07c-31b3-43e0-b660-867780cbaba5.png)

FFVideo Player supports multiple simultanious minimum delay playback video windows, seek, scrubbing, basic face detection, plus the surrounding code necessary for a moderately
professional application, such as persistance for end-user settings, and this embedded web browser providing a 'help window'.

The idea of this application is to provide a basic video app framework for developers wanting to learn, and experiment with video filters without the overhead of audio processing
or the normal frame delays of ordinary video playback. If one is training models with video data, and want to preserve the maximum amount of resources for training, this provides
a nice framework for doing so.

Some of the coding examples in this project include:
 - An FFmpeg library supporting 
   - video files, USB Cameras, IP Cameras and IP video services
   - video file seeks, scrubbing, AVFilterGrph filters, and frame exporting
   - This project uses the author's modified FFmpeg located here 
     - https://github.com/bsenftner/FFmpeg
     - The modification is to support unexpected IP and USB stream dropouts
   - This project also uses the author's SQLite3 wrapper library, located here:
     - https://github.com/bsenftner/kvs
 - A multi-threaded wxWidgets Video Player application
   - Multiple simultanious video windows
   - Exported video frames collected and re-encoded as H.264 .MP4 and .264 Elementary Streams 
   - Easy access to AVFilterGraph Video Filters and video experimentation
   - Integration with Dlib and a basic example of Face Detection of Face Landmark Recovery
   - An embedded web browser as the "help" window
   - Lots in-code of documentation describing how, what and why 

