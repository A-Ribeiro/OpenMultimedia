# OpenMultimedia

> For this is how God loved the world:  
he gave his only Son, so that everyone  
who believes in him may not perish  
but may have eternal life.  
  \
John 3:16

This project was born in 2017 with the name CameraStreamer. It was my personal attempt to create a real-time video streaming to be applicable in the Catholic parish or the Catholic Church.

The current version is more flexible than the 1.x versions (2017).

With the pandemic, I had more time to upgrade the original project's codebase.

# What does it do?

This is a framework written in C++ to allow the transmission of image streams over the network.

There is an android app that streams the camera image.

It is possible to use the image stream as virtual cameras within real-time video editing software like OBS.

# Main Features

- Fully rethought operating architecture;
- Now the android application uses the MediaRecorder API;
- Image compression modes are selected according to what is supported and enabled within the smartphone HW/API;
- New viewers are compatible with DirectShow, V4L2 Loopback and Syphon;
- The device discovery algorithm uses IPv4 broadcast from the device to the active lan/wlan interfaces.

## Components

This implementation split the logic of operation into pieces (binaries).

There are the network layer and the presentation layer.

All components comunicate with each other using IPC channels (Inter Process Communication) thanks to https://github.com/A-Ribeiro/aRibeiroPlatform .

### Network Layer

Instead of using the default protocols wrappers, this implementation uses RAW TCP packets.

The android application wait TCP connections to deliver the stream to the network.

The __network_to_ipc__ connects to the android device and reads the stream from the network.

The data is decoded and sent to the IPC channel.

### Presentation Layer

The visualizers reads the IPC channel and present it to the user.

The __ipc_dshow_source__ creates a virtual webcam (aRibeiro Cam 01) with the image from the IPC channel.

The __ipc-fullscreen-viewer__ creates an OpenGL fullscreen window rendering the image from the IPC channel.

## NOTE: Fullscreen Viewer

The fullscreen viewer is part of this package, but as it uses the https://github.com/A-Ribeiro/OpenGLStarter framework/engine,
the code is in the other repository (to be easy to build the binary).

## Authors

***Alessandro Ribeiro da Silva*** obtained his Bachelor's degree in Computer Science from Pontifical Catholic 
University of Minas Gerais and a Master's degree in Computer Science from the Federal University of Minas Gerais, 
in 2005 and 2008 respectively. He taught at PUC and UFMG as a substitute/assistant professor in the courses 
of Digital Arts, Computer Science, Computer Engineering and Digital Games. He have work experience with interactive
software. He worked with OpenGL, post-processing, out-of-core rendering, Unity3D and game consoles. Today 
he work with freelance projects related to Computer Graphics, Virtual Reality, Augmented Reality, WebGL, web server 
and mobile apps (andoid/iOS).

More information on: https://alessandroribeiro.thegeneralsolution.com/
