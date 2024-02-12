#include <InteractiveToolkit/InteractiveToolkit.h>

#include "v4l2.h"

#include <sys/ioctl.h>
#include <dirent.h>
#include <unistd.h> // readLink, write


void signal_handler(int signal) {
    printf("   ****************** signal_handler **********************\n");
    Platform::Thread::getMainThread()->interrupt();
}

Device queryDeviceByName(const char* name) {
    std::vector<Device> devices = v4l2::listDevices();
    for(int i=0;i<devices.size();i++){
        if (strcmp(name, (char*)devices[i].capability.card) == 0)
            return devices[i];
    }
    ITK_ABORT(true, "Device not found: %s\n", name);
    return Device();
}

#define ROUND_UP_2(num)  (((num)+1)&~1)

int main(int argc, char* argv[]){
    Platform::Signal::Set(signal_handler);
    ITKCommon::Path::setWorkingPath(ITKCommon::Path::getExecutablePath(argv[0]));
    Platform::Thread::staticInitialization();

    Device device;
    device = queryDeviceByName("aRibeiro Cam 01");

    device.open();
    device.setWriteFormat( 1920, 1080, 30, V4L2_PIX_FMT_YUYV, 1920 * 2, 1920 * 1080 * 2 );

    Platform::IPC::LowLatencyQueueIPC yuy2_queue( "aRibeiro Cam 01", Platform::IPC::QueueIPC_READ, 8, 1920 * 1080 * 2);
    Platform::ObjectBuffer data_buffer;
    data_buffer.setSize(1920 * 1080 * 2);

    while (!Platform::Thread::isCurrentThreadInterrupted()) {
        
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