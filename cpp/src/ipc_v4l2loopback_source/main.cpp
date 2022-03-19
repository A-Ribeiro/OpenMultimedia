#include <aRibeiroCore/aRibeiroCore.h>
#include <aRibeiroPlatform/aRibeiroPlatform.h>
#include <aRibeiroData/aRibeiroData.h>
using namespace aRibeiro;

#include "v4l2.h"

void signal_handler(int signal) {
    printf("   ****************** signal_handler **********************\n");
    PlatformThread::getMainThread()->interrupt();
}

Device queryDeviceByName(const char* name) {
    std::vector<Device> devices = v4l2::listDevices();
    for(int i=0;i<devices.size();i++){
        if (strcmp(name, (char*)devices[i].capability.card) == 0)
            return devices[i];
    }
    ARIBEIRO_ABORT(true, "Device not found: %s\n", name);
    return Device();
}

int main(int argc, char* argv[]){
    PlatformSignal::Set(signal_handler);
    PlatformPath::setWorkingPath(PlatformPath::getExecutablePath(argv[0]));
    // initialize self referencing of the main thread.
    PlatformThread::getMainThread();

    Device device;
    device = queryDeviceByName("aRibeiro Cam 01");
    //v4l2::loadDeviceInfo("/dev/video0",&device);

    //ARIBEIRO_ABORT( !(device.capability.capabilities & V4L2_CAP_VIDEO_CAPTURE),
    //                "device is not a video capture device.\n")

    v4l2_fmtdesc formatDescription;
    ARIBEIRO_ABORT( !device.queryPixelFormat(V4L2_PIX_FMT_YUYV, &formatDescription),
                    "device doesn't support YUYV reading.\n")

    v4l2_frmsizeenum res;
    ARIBEIRO_ABORT( !device.queryNearResolutionForFormat(formatDescription, 1920, 1080, &res),
                    "could not query resolution.\n")

    ARIBEIRO_ABORT( (res.discrete.width!= 1920) || (res.discrete.height != 1080),
                    "Cannot open with resolution: 1920x1080.\n")

    v4l2_frmivalenum interval;
    ARIBEIRO_ABORT( !device.queryNearInterval(res, 30.0f, &interval),
                    "could not query interval of 30 fps.\n")
    
    fprintf(stderr,"%s -> %s\n", device.path.c_str(), device.capability.card );
    fprintf(stderr,"  %s", fcc2s(formatDescription.pixelformat).c_str() );
    fprintf(stderr," / %u x %u", res.discrete.width, res.discrete.height );
    fprintf(stderr," @ %s fps\n", fract2fps( interval.discrete ).c_str() );

    device.open();
    device.setFormat(formatDescription, res, interval);

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

    return 0;
}