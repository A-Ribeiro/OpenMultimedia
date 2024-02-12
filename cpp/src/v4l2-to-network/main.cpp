#include <InteractiveToolkit/InteractiveToolkit.h>

#include "NetworkDiscovery.h"
#include "NetworkTCPServer.h"

#include "v4l2.h"

const int FORMAT_YUV2 = 1; //Android default image format
const int FORMAT_YUY2 = 2; //OBS channel image format

const int FORMAT_YUV420P = 3;
const int FORMAT_YUV420P_ZLIB = 100;

const int FORMAT_H264 = 1000;
const int FORMAT_HEVC = 1001;
const int FORMAT_3GPP = 1002;
const int FORMAT_MJPEG = 1003;

void signal_handler(int signal)
{
    printf("   ****************** signal_handler **********************\n");
    Platform::Thread::getMainThread()->interrupt();
}

void listDevices()
{
    printf("Listing MJPEG compatible devices:\n");
    std::vector<Device> devices = v4l2::listDevices();
    for (int i = 0; i < devices.size(); i++)
    {
        if (devices[i].capability.capabilities & (V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING))
        {
            v4l2_fmtdesc formatDescription;
            if (devices[i].queryPixelFormat(V4L2_PIX_FMT_MJPEG, &formatDescription))
            {
                printf("    \"%s\"\n", (char *)devices[i].capability.card);
            }
        }
    }
}

Device queryDeviceByName(const char *name)
{
    std::vector<Device> devices = v4l2::listDevices();
    for (int i = 0; i < devices.size(); i++)
    {
        if (strcmp(name, (char *)devices[i].capability.card) == 0)
            return devices[i];
    }
    ITK_ABORT(true, "Device not found: %s\n", name);
    return Device();
}

#define ROUND_UP_2(num) (((num) + 1) & ~1)

int main(int argc, char *argv[])
{
    Platform::Signal::Set(signal_handler);
    ITKCommon::Path::setWorkingPath(ITKCommon::Path::getExecutablePath(argv[0]));
    Platform::Thread::staticInitialization();

    if (argc == 2){
        std::string device_name = argv[1];

        printf("Trying to open: %s\n", device_name.c_str());
        Device device;
        device = queryDeviceByName(device_name.c_str());

        v4l2_fmtdesc formatDescription;
        if (! device.queryPixelFormat(V4L2_PIX_FMT_MJPEG, &formatDescription) ){
            fprintf(stderr,"device doesn't support MJPEG reading.\n");
            exit(-1);
        }

        v4l2_frmsizeenum res;
        if (!device.queryNearResolutionForFormat(formatDescription, 1920, 1080, &res)){
            fprintf(stderr,"could not query resolution.\n");
            exit(-1);
        }

        v4l2_frmivalenum interval;
        if (!device.queryNearInterval(res, 30.0f, &interval)){
            fprintf(stderr,"could not query interval of 30 fps.\n");
            exit(-1);
        }

        fprintf(stderr,"    %s -> %s\n", device.path.c_str(), device.capability.card );
        fprintf(stderr,"    %s", fcc2s(formatDescription.pixelformat).c_str() );
        fprintf(stderr," / %u x %u", res.discrete.width, res.discrete.height );
        fprintf(stderr," @ %s fps\n", fract2fps( interval.discrete ).c_str() );


        device.open();

        //v4l2_queryctrl ctrl;
        //if ( device.queryControlByName("Brightness", &ctrl ) ){
            //device.setCtrlValue(ctrl, 128);
        //}

        device.printControls();

        v4l2_queryctrl ctrl;
        if ( device.queryControlByName("Exposure, Auto", &ctrl ) && (ctrl.type == V4L2_CTRL_TYPE_MENU) ){
            int v;
            if (device.queryMenuByName(ctrl, "Aperture Priority Mode", &v)){
                device.setCtrlValue(ctrl, v);
                printf("Exposure, Auto = Aperture Priority Mode\n");
            }
        }

        if ( device.queryControlByName("Focus, Auto", &ctrl ) && (ctrl.type == V4L2_CTRL_TYPE_BOOLEAN) ){
            device.setCtrlValue(ctrl, 1);
            printf("Focus, Auto = 1\n");
        }

        if ( device.queryControlByName("Exposure, Auto Priority", &ctrl ) && (ctrl.type == V4L2_CTRL_TYPE_BOOLEAN) ){
            device.setCtrlValue(ctrl, 0);
            printf("Exposure, Auto Priority = 0\n");
        }

        device.setFormat(formatDescription, res, interval);

        // buffer allocation and information retrieve
        const int BUFFER_COUNT = 3; // max 32

        device.setNumberOfInputBuffers(BUFFER_COUNT);
        v4l2_buffer bufferInfo[BUFFER_COUNT];
        void* bufferPtr[BUFFER_COUNT];
        for(int i=0;i<BUFFER_COUNT;i++) {
            bufferInfo[i] = device.getBufferInformationFromDevice(i);
            bufferPtr[i] = device.getBufferPointer(bufferInfo[i]);
            if (bufferInfo[i].length > 0)
                memset(bufferPtr[i], 0, bufferInfo[i].length);
            device.queueBuffer(i, &bufferInfo[i]);
        }

        //main loop
        device.streamON();
        printf("Stream Started...\n");

        int fd_stdout = fileno(stdout);
        v4l2_buffer bufferQueue;


        NetworkDiscovery networkDiscovery("v4l2");
        NetworkTCPServer networkTCPServer;

        networkDiscovery.start();
        networkTCPServer.start();
        

        while (!Platform::Thread::isCurrentThreadInterrupted()){
            if (!device.dequeueBuffer(&bufferQueue))
                break;

            //output to stdout
            //write(fd_stdout,bufferPtr[bufferQueue.index],bufferQueue.bytesused);
            networkTCPServer.write(
                (uint8_t*)bufferPtr[bufferQueue.index],bufferQueue.bytesused,
                res.discrete.width, res.discrete.height,
                FORMAT_MJPEG
            );

            if (!device.queueBuffer(bufferQueue.index, &bufferQueue))
                break;
        }

        device.streamOFF();
        device.close();

        networkTCPServer.stop();
        networkDiscovery.stop();

        printf("Stream Stopped.\n");


    } else {
        listDevices();
    }

    /*
    Device device;
    device = queryDeviceByName("aRibeiro Cam 01");

    device.open();
    device.setWriteFormat( 1920, 1080, 30, V4L2_PIX_FMT_YUYV, 1920 * 2, 1920 * 1080 * 2 );

    aRibeiro::PlatformLowLatencyQueueIPC yuy2_queue( "aRibeiro Cam 01", PlatformQueueIPC_READ, 8, 1920 * 1080 * 2);
    aRibeiro::ObjectBuffer data_buffer;
    data_buffer.setSize(1920 * 1080 * 2);

    while (!PlatformThread::isCurrentThreadInterrupted()) {
        
        if (yuy2_queue.read(&data_buffer)) {
            int size_check = 1920 * 1080 * 2;
            if (data_buffer.size != size_check)
                continue;

            device.write(data_buffer.data, data_buffer.size);
        }
    }

    device.close();
*/

    return 0;
}