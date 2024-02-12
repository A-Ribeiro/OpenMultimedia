#ifndef v4l2__H
#define v4l2__H

//
// // Example of use:
//
// #include "v4l2.h"
// Device device;
// v4l2::loadDeviceInfo("/dev/video0",&device);

// if ( !(device.capability.capabilities & V4L2_CAP_VIDEO_CAPTURE) ){
//     fprintf(stderr,"device is not a video capture device.\n");
//     exit(-1);
// }

// if ( !(device.capability.capabilities & V4L2_CAP_STREAMING) ){
//     fprintf(stderr,"device doesn't support streaming.\n");
//     exit(-1);
// }

// v4l2_fmtdesc formatDescription;
// if (! device.queryPixelFormat(V4L2_PIX_FMT_YUYV, &formatDescription) ){
// //if (! device.queryPixelFormat(V4L2_PIX_FMT_MJPEG, &formatDescription) ){
//     fprintf(stderr,"device doesn't support H264 reading.\n");
//     exit(-1);
// }

// v4l2_frmsizeenum res;
// if (!device.queryNearResolutionForFormat(formatDescription, 1280, 720, &res)){
//     fprintf(stderr,"could not query resolution.\n");
//     exit(-1);
// }

// v4l2_frmivalenum interval;
// if (!device.queryNearInterval(res, 30.0f, &interval)){
//     fprintf(stderr,"could not query interval of 24 fps.\n");
//     exit(-1);
// }

// fprintf(stderr,"%s -> %s\n", device.path.c_str(), device.capability.card );
// fprintf(stderr,"  %s", fcc2s(formatDescription.pixelformat).c_str() );
// fprintf(stderr," / %u x %u", res.discrete.width, res.discrete.height );
// fprintf(stderr," @ %s fps\n", fract2fps( interval.discrete ).c_str() );

// device.open();

// //v4l2_queryctrl ctrl;
// //if ( device.queryControlByName("Brightness", &ctrl ) ){
//     //device.setCtrlValue(ctrl, 128);
// //}

// device.printControls();
// device.setFormat(formatDescription, res, interval);

// // buffer allocation and information retrieve
// const int BUFFER_COUNT = 3; // max 32

// device.setNumberOfInputBuffers(BUFFER_COUNT);
// v4l2_buffer bufferInfo[BUFFER_COUNT];
// void* bufferPtr[BUFFER_COUNT];
// for(int i=0;i<BUFFER_COUNT;i++) {
//     bufferInfo[i] = device.getBufferInformationFromDevice(i);
//     bufferPtr[i] = device.getBufferPointer(bufferInfo[i]);
//     memset(bufferPtr[i], 0, bufferInfo[i].length);
//     device.queueBuffer(i, &bufferInfo[i]);
// }

// //main loop
// device.streamON();

// int fd_stdout = fileno(stdout);
// v4l2_buffer bufferQueue;

// while (!exit_requested){
//     device.dequeueBuffer(&bufferQueue);
//     //output to stdout
//     write(fd_stdout,bufferPtr[bufferQueue.index],bufferQueue.bytesused);
//     device.queueBuffer(bufferQueue.index, &bufferQueue);
// }

// device.streamOFF();
// device.close();



/*
static int xioctl(int fh, int request, void *arg)
{
    int r;
    do {
        r = ioctl(fh, request, arg);
    } while (-1 == r && EINTR == errno);
    return r;
}
*/
#include <linux/v4l2-common.h>
#include <linux/v4l2-controls.h>
#include <linux/videodev2.h>

#include <sys/mman.h>
#include <fcntl.h> // open

#include <vector>
#include <map>
#include <string>
#include <locale> // tolower


static std::string fcc2s(unsigned int val)
{
	std::string s;

	s += val & 0x7f;
	s += (val >> 8) & 0x7f;
	s += (val >> 16) & 0x7f;
	s += (val >> 24) & 0x7f;
	if (val & (1 << 31))
		s += "-BE";
	return s;
}

static std::string fract2sec(const struct v4l2_fract &f)
{
	char buf[100];

	sprintf(buf, "%.3f", (1.0 * f.numerator) / f.denominator);
	return buf;
}

static std::string fract2fps(const struct v4l2_fract &f)
{
	char buf[100];

	sprintf(buf, "%.3f", (1.0 * f.denominator) / f.numerator);
	return buf;
}


static std::string controlType2s(__u32 t){
    switch(t){
        case V4L2_CTRL_TYPE_INTEGER:      return "int";
        case V4L2_CTRL_TYPE_INTEGER64:    return "int64";
        case V4L2_CTRL_TYPE_STRING:       return "str";
        case V4L2_CTRL_TYPE_BOOLEAN:      return "bool";
        case V4L2_CTRL_TYPE_MENU:         return "menu";
        case V4L2_CTRL_TYPE_INTEGER_MENU: return "intmenu";
        case V4L2_CTRL_TYPE_BUTTON:       return "button";
        case V4L2_CTRL_TYPE_BITMASK:      return "bitmask";
        case V4L2_CTRL_TYPE_U8:           return "u8";
        case V4L2_CTRL_TYPE_U16:          return "u16";
        case V4L2_CTRL_TYPE_U32:          return "u32";
        default:                          return "unknown";
    }
}


class Device{
    bool streaming;
public:
    std::string path;
    v4l2_capability capability;
    std::vector<v4l2_fmtdesc> supportedFormats;

    std::vector<v4l2_queryctrl> controls;

    bool queryControlByName(const std::string &name, v4l2_queryctrl *result);

    //V4L2_PIX_FMT_H264
    bool queryPixelFormat(__u32 pixelFormat, v4l2_fmtdesc *output);
    bool queryNearResolutionForFormat(const v4l2_fmtdesc &format, __u32 width, __u32 height, v4l2_frmsizeenum *output);
    bool queryNearInterval(const v4l2_frmsizeenum &res, float fps, v4l2_frmivalenum *output);


    int fd;

    Device();
    ~Device();

    //
    // online device handling
    //
    bool open();
    void close();
    void setWriteFormat(int width, int height, int fps, uint32_t pixelformat, int line_size_bytes, int image_size_bytes);
    void write(const uint8_t* buffer, int size);

    void setFormat(const v4l2_fmtdesc &fmt, const v4l2_frmsizeenum &res, const v4l2_frmivalenum &interval );
    void setNumberOfInputBuffers(int count);
    v4l2_buffer getBufferInformationFromDevice(int bufferIndex);
    void* getBufferPointer(const v4l2_buffer &bufferinfo);

    bool queueBuffer(int bufferindex, v4l2_buffer *bufferinfo);
    bool dequeueBuffer(v4l2_buffer *bufferinfo);

    void streamON();
    void streamOFF();

    void printControls();

    int getCtrlValue(const v4l2_queryctrl &qctrl);
    void setCtrlValue(const v4l2_queryctrl &qctrl, int v);


    bool queryMenuByName(const v4l2_queryctrl &qctrl, const char* name, int *output);

};

class v4l2{
public:
    //can return an invalid device path name
    static std::vector<std::string> listDevicesNames();

    //return the correct device listing with its capabilities...
    static std::vector<Device> listDevices();

    static bool loadDeviceInfo(const std::string &path, Device *device);

    static bool getDeviceCapability(const std::string &device, v4l2_capability *vcap);
    static std::vector<v4l2_fmtdesc> getDeviceFormats(const std::string &device);
    static std::vector<v4l2_frmsizeenum> getFramesizes(const std::string &device, const v4l2_fmtdesc &fmt);
    static std::vector<v4l2_frmivalenum> getFrameIntervals(const std::string &device, const v4l2_frmsizeenum &framesize);

    static std::vector<v4l2_queryctrl> getControls(const std::string &device);


};


#endif
