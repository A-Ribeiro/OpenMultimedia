#ifndef __ImageTransmitterReceiver__h__
#define __ImageTransmitterReceiver__h__

#include <aRibeiroCore/aRibeiroCore.h>
#include <aRibeiroPlatform/aRibeiroPlatform.h>
using namespace aRibeiro;

const int FORMAT_YUV2 = 1;//Android default image format
const int FORMAT_YUY2 = 2;//OBS channel image format

const int FORMAT_YUV420P = 3;
const int FORMAT_YUV420P_ZLIB = 100;

const int FORMAT_H264 = 1000;
const int FORMAT_HEVC = 1001;
const int FORMAT_3GPP = 1002;
//const int FORMAT_VP9 = 1003;


static uint32_t compute_YUV420_888_FormatSize(int width, int height) {
    //compute buffer size for NV21 or YV12
    int w_aligned_16 = (width / 2) * 2;//((int)((float)width / 16.0 + 0.5)) * 16;
    int h_aligned_16 = (height / 2) * 2;// ((int)((float)height / 16.0 + 0.5)) * 16;
    int h_uv_aligned_16 = h_aligned_16 / 2;// ((int)(((float)h_aligned_16 / 2.0) / 16.0 + 0.5)) * 16;

    int yStride = w_aligned_16;
    int uvStride = yStride/2;// ((int)(((float)yStride / 2.0) / 16.0 + 0.5)) * 16;
    int ySize = yStride * h_aligned_16;
    int uvSize = uvStride * h_uv_aligned_16;

    int size = ySize + uvSize * 2;

    return (uint32_t)size;
}


DefineMethodPointer(ImageReceiver_OnData, 
    void,
    const uint8_t *data, size_t data_size,
    uint32_t width, uint32_t height,
    uint32_t format) VoidMethodCall(data, data_size, width, height, format)



class NetworkImageReceiver {

    PlatformThread thread;

    public:

    ImageReceiver_OnData onData;
    std::string force_connection_to_ip;

    NetworkImageReceiver():thread(this, &NetworkImageReceiver::run) {

    }

    void start() {
        thread.start();
    }

    void close() {
        thread.interrupt();
        thread.wait();
    }

    void wait() {
        thread.wait();
    }

    void run() {

        PlatformSocketUDP udp_discover_smartphone;
        PlatformSocketTCP tcp_data_receiver;
        ObjectBuffer data_buffer;

        while ( !PlatformThread::isCurrentThreadInterrupted() ) {

            std::string device_addr;

            if (force_connection_to_ip.size() == 0) {

                printf("[ImageReceiver] discovering stream...\n");

                //initialize discover smartphone
                udp_discover_smartphone.createFD(true, true, 64);
                //udp_discover_smartphone.multicastAddMembership("239.0.0.1");
                udp_discover_smartphone.bind("INADDR_ANY", NetworkConstants::PUBLIC_PORT_MULTICAST_START);

                struct sockaddr_in device_addr_aux;

                char input_data[1024];
                memset(input_data, 0, sizeof(char) * 1024);
                uint32_t read_feedback;
                if (!udp_discover_smartphone.read_buffer(&device_addr_aux, (uint8_t*)input_data, 1024, &read_feedback)) {
                    //udp_discover_smartphone.multicastDropMembership("239.0.0.1");
                    udp_discover_smartphone.close();
                    break;
                }

                //udp_discover_smartphone.multicastDropMembership("239.0.0.1");
                udp_discover_smartphone.close();

                if (!StringUtil::startsWith(input_data, "OpenMultimediaVideoSource:")) {
                    printf("[ImageReceiver] Not a Valid Stream...\n");
                    PlatformSleep::sleepMillis(100);
                    continue;
                }

                device_addr = inet_ntoa(device_addr_aux.sin_addr);

                std::string device_name = &input_data[26];

                printf("[ImageReceiver] Found device: %s -> %s\n", device_name.c_str(), device_addr.c_str());

            }
            else {
                device_addr = force_connection_to_ip;
            }

            printf("[ImageReceiver] trying to connect ...\n");

            if ( !tcp_data_receiver.connect( device_addr , NetworkConstants::PUBLIC_PORT_START) ) {
                tcp_data_receiver.close();// free resources to try to connect in the future...
                printf("[ImageReceiver] ERROR...\n");
                PlatformSleep::sleepMillis(500);
                continue;
            }

            printf("[ImageReceiver] CONNECTED!!!\n");
            tcp_data_receiver.setNoDelay(true);
            tcp_data_receiver.setReadTimeout(5000);//5 secs with no activities will disconnect...

            while (!tcp_data_receiver.isClosed() && !tcp_data_receiver.isSignaled() ) {

                if (!tcp_data_receiver.read_semaphore.blockingAcquire())
                    break;

                uint32_t buffer_size_bytes;
                uint32_t width;
                uint32_t height;
                uint32_t format;

                if (!tcp_data_receiver.read_buffer((uint8_t*)&buffer_size_bytes,sizeof(uint32_t))){
                    printf("[ImageReceiver] error reading buffer_size_bytes\n");
                    tcp_data_receiver.read_semaphore.release();
                    break;
                }
                if (!tcp_data_receiver.read_buffer((uint8_t*)&width,sizeof(uint32_t))){
                    printf("[ImageReceiver] error reading width\n");
                    tcp_data_receiver.read_semaphore.release();
                    break;
                }
                if (!tcp_data_receiver.read_buffer((uint8_t*)&height,sizeof(uint32_t))){
                    printf("[ImageReceiver] error reading height\n");
                    tcp_data_receiver.read_semaphore.release();
                    break;
                }
                if (!tcp_data_receiver.read_buffer((uint8_t*)&format,sizeof(uint32_t))){
                    printf("[ImageReceiver] error reading format\n");
                    tcp_data_receiver.read_semaphore.release();
                    break;
                }

                if (format == FORMAT_YUV420P) {
                    uint32_t cross_check_image_size_bytes = compute_YUV420_888_FormatSize(width, height);

                    if (cross_check_image_size_bytes != buffer_size_bytes) {
                        printf("[ImageReceiver] Cross Check Wrong...\n");
                        tcp_data_receiver.read_semaphore.release();
                        break;
                    }
                } else { //all other formats are compressed... so the size must be less than the original image size.
                    //if (format == FORMAT_H264 || format == FORMAT_YUV420P_ZLIB) 
                    uint32_t cross_check_image_size_bytes = compute_YUV420_888_FormatSize(width, height);
                    if (buffer_size_bytes > cross_check_image_size_bytes) {
                        printf("[ImageReceiver] Compressed Buffer greater then the original image... Cross Check Wrong...\n");
                        tcp_data_receiver.read_semaphore.release();
                        break;
                    }
                }
                

                data_buffer.setSize( buffer_size_bytes );

                if (!tcp_data_receiver.read_buffer(data_buffer.data,data_buffer.size)){
                    printf("[ImageReceiver] error reading data_buffer\n");
                    tcp_data_receiver.read_semaphore.release();
                    break;
                }

                tcp_data_receiver.read_semaphore.release();

                //
                // send the buffer to the external process
                //
#ifndef NDEBUG
                //static int count = 0;
                //printf("data (%i, %i) %i -> size %i\n", width, height, count++, data_buffer.size);
#endif
                if (onData != NULL)
                    onData(data_buffer.data,data_buffer.size, width, height, format);
            }

            printf("[ImageReceiver] Closing connection...\n");
            tcp_data_receiver.close();
        }


        printf("[ImageReceiver] Finalizing Processing Thread...\n");
    }


};

#endif