#include <aRibeiroCore/aRibeiroCore.h>
#include <aRibeiroPlatform/aRibeiroPlatform.h>
#include <aRibeiroData/aRibeiroData.h>
using namespace aRibeiro;

#include "v4l2.h"

#include <sys/ioctl.h>
#include <dirent.h>
#include <unistd.h> // readLink, write


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

#define ROUND_UP_2(num)  (((num)+1)&~1)

int main(int argc, char* argv[]){
    PlatformSignal::Set(signal_handler);
    PlatformPath::setWorkingPath(PlatformPath::getExecutablePath(argv[0]));
    // initialize self referencing of the main thread.
    PlatformThread::getMainThread();

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

    return 0;
}