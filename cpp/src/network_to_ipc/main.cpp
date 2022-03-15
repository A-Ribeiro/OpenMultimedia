#ifdef _MSC_VER
#include <io.h>
//#    if defined(NDEBUG)
#        pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
//#    else
//#        pragma comment(linker, "/SUBSYSTEM:CONSOLE")
//#    endif
#endif

#include <zlib.h>

#include <aRibeiroCore/aRibeiroCore.h>
#include <aRibeiroPlatform/aRibeiroPlatform.h>
#include <aRibeiroData/aRibeiroData.h>
using namespace aRibeiro;

#include <ffmpeg-wrapper/ffmpeg-wrapper.h>
using namespace FFmpegWrapper;
FFmpegVideoDecoder* decoder = NULL;

#include "NetworkImageReceiver.h"
#include "colorspace_ops.h"

void signal_handler(int signal) {
    printf("   ****************** signal_handler **********************\n");
    PlatformThread::getMainThread()->interrupt();
}

int w, h, chann, pixDepth;
uint8_t* yuy2, *yuy2_aux;
PlatformMutex mutex;
ObjectBuffer zlib_output;
CopyRescaleMultithread* copyRescale = NULL;

void swapAuxToYUY2() {
    PlatformAutoLock autoLock(&mutex);
    uint8_t* tmp = yuy2_aux;
    yuy2_aux = yuy2;
    yuy2 = tmp;
}

void On_DataYUV420P(int width, int height, const uint8_t *data, size_t size) {
    copyRescale->yuv420_to_yuy2_copy_rescale(data, width, height, yuy2_aux, w, h);
    swapAuxToYUY2();
}

void On_Data(const uint8_t *data, size_t data_size,
    uint32_t width, uint32_t height,
    uint32_t format) {

    FFmpegWrapper::VideoType videoType = FFmpegWrapper::VideoType_NONE;

    if (format == FORMAT_HEVC)
        videoType = FFmpegWrapper::VideoType_HEVC;
    else if (format == FORMAT_H264)
        videoType = FFmpegWrapper::VideoType_H264;
    else if (format == FORMAT_3GPP)
        videoType = FFmpegWrapper::VideoType_3GPP;

    if (videoType != FFmpegWrapper::VideoType_NONE){
        if (decoder->isInitialized() && decoder->videoType != videoType)
            decoder->release();
        if (!decoder->isInitialized()){
            decoder->initialize( videoType );
            decoder->onData = On_DataYUV420P;
        }
        decoder->sendData(data, data_size);
    }
    else if (format == FORMAT_YUV420P_ZLIB) {
        //uncompress
        uLongf final_size = compute_YUV420_888_FormatSize(width, height);
        zlib_output.setSize(final_size);

        int result = ::uncompress((Bytef *)zlib_output.data,
            &final_size,
            data,
            data_size);
        if (result == Z_OK) {
            PlatformAutoLock autoLock(&mutex);
            copyRescale->yuv420_to_yuy2_copy_rescale(zlib_output.data, width, height, yuy2_aux, w, h);
            swapAuxToYUY2();
        }

    } else if (format == FORMAT_YUV420P) {
        PlatformAutoLock autoLock(&mutex);
        copyRescale->yuv420_to_yuy2_copy_rescale(data, width, height, yuy2_aux, w, h);
        swapAuxToYUY2();
    }

}

int main(int argc, char* argv[]){

#ifdef _MSC_VER
    if (argc != 2 || strcmp(argv[1], "-noconsole") != 0) {
        //alloc win32 console...
        if (!AttachConsole(ATTACH_PARENT_PROCESS))
            AllocConsole();
        AttachConsole(ATTACH_PARENT_PROCESS);

        /*
        CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
        int consoleHandleR, consoleHandleW;
        long stdioHandle;
        FILE* fptr;

        //SetConsoleTitle("Dev Console");

        EnableMenuItem(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE, MF_GRAYED);
        DrawMenuBar(GetConsoleWindow());

        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &consoleInfo);

        stdioHandle = (long)GetStdHandle(STD_INPUT_HANDLE);
        consoleHandleR = _open_osfhandle(stdioHandle, _O_TEXT);
        fptr = _fdopen(consoleHandleR, "r");
        *stdin = *fptr;
        setvbuf(stdin, NULL, _IONBF, 0);

        stdioHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);
        consoleHandleW = _open_osfhandle(stdioHandle, _O_TEXT);
        fptr = _fdopen(consoleHandleW, "w");
        *stdout = *fptr;
        setvbuf(stdout, NULL, _IONBF, 0);

        stdioHandle = (long)GetStdHandle(STD_ERROR_HANDLE);
        *stderr = *fptr;
        setvbuf(stderr, NULL, _IONBF, 0);
        */

        //if (AttachConsole(ATTACH_PARENT_PROCESS))
        //AttachConsole(ATTACH_PARENT_PROCESS);
        {
            FILE* fpstdin = stdin, * fpstdout = stdout, * fpstderr = stderr;

            freopen_s(&fpstdin, "CONIN$", "r", stdin);
            freopen_s(&fpstdout, "CONOUT$", "w", stdout);
            freopen_s(&fpstderr, "CONOUT$", "w", stderr);

            //std::cout << "This is a test of the attached console" << std::endl;
            //FreeConsole();
        }
        /*
HANDLE myConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
DWORD cCharsWritten;
char* str = TEXT("enter game command");
WriteConsole(myConsoleHandle, str, strlen(str), &cCharsWritten, NULL);
char* command = malloc(100);
int charsRead = 0;
ReadConsole(myConsoleHandle, command, 100, &cCharsRead, NULL);
        */
    }
#endif

    // make only one instance running per time
    PlatformBufferIPC bufferIPC("network-to-ipc", PlatformBufferIPC_READ | PlatformBufferIPC_WRITE, 0);
    if (bufferIPC.isFirstProcess())
        bufferIPC.finishInitialization();
    else {
        bufferIPC.finishInitialization();
        return 0;
    }


#ifdef _MSC_VER
    PlatformProcessGracefulWindowsProcessTerminator windowsCustomSignalHandler(signal_handler);
#endif

    PlatformSignal::Set(signal_handler);
    PlatformPath::setWorkingPath(PlatformPath::getExecutablePath(argv[0]));
    // initialize self referencing of the main thread.
    PlatformThread::getMainThread();

    int num_of_system_threads = PlatformThread::QueryNumberOfSystemThreads();

    num_of_system_threads /= 2;

    copyRescale = new CopyRescaleMultithread(num_of_system_threads,num_of_system_threads*4);
    decoder = new FFmpegVideoDecoder();

    printf("network-to-ipc\n"
        "\n"
        "For this is how God loved the world :\n"
        "he gave his only Son, so that everyone\n"
        "who believes in him may not perish\n"
        "but may have eternal life.\n"
        "\n"
        "John 3:16\n\n");

    char* img = PNGHelper::readPNG("background_no_connection.png",&w,&h,&chann,&pixDepth);

    ARIBEIRO_ABORT(chann != 3 && chann != 4, "Background image must have 3 or 4 components (RGB or RGBA)")

    uint8_t* rgbx_img = NULL;
    if (chann == 3) {
        rgbx_img = alloc_rgb_to_rgbx_aligned((uint8_t*)img, w, h);
        PNGHelper::closePNG(img);
    }
    else if (chann == 4) {
        rgbx_img = (uint8_t*)img;
    }
    //uint8_t* yuy2 = alloc_yuy2_aligned(w,h);
    yuy2 = alloc_yuy2_aligned(w, h);
    yuy2_aux = alloc_yuy2_aligned(w, h);
    rgbx_to_yuy2(rgbx_img, yuy2, w, h);

    if (chann == 3)
        free_aligned(rgbx_img);
    else if (chann == 4)
        PNGHelper::closePNG(img);

    //save binary version of the image
    /*
    BinaryWriter writer;
    writer.writeToFile("image.zlib");
    writer.writeBuffer(yuy2, w*h*2);
    writer.close();
    */

    int interval = 1000/30 + 1;

    NetworkImageReceiver receiver;
    receiver.onData = On_Data;
    if (argc == 2 && strcmp(argv[1], "-noconsole") != 0)
        receiver.force_connection_to_ip = std::string(argv[1]);
    receiver.start();

    PlatformTime timer;

    PlatformLowLatencyQueueIPC queue( "aRibeiro Cam 01", PlatformQueueIPC_WRITE, 8, 1920*1080*2 );
    int count = 0;
    while (!PlatformThread::isCurrentThreadInterrupted()) {
        interval = 1000/30 - 1;

        if (queue.writeHasEnoughSpace(w* h * 2, true)) {
            PlatformAutoLock autoLock(&mutex);
            //printf("send image: %i\n", count++);
            queue.write(yuy2, w*h * 2, false, true);
        }
        else {
            //printf(".");
            //fflush(stdout);
        }

        timer.update();
        int passed_time_ms = timer.deltaTimeMicro/1000;
       
        if (passed_time_ms < interval)
            PlatformSleep::sleepMillis(interval - passed_time_ms);
        timer.update();
    }

    printf("receiver.close\n");
    receiver.close();
    receiver.onData = NULL;

    copyRescale->finalizeThreads();

    decoder->release();
    printf("delete decoder\n");
    delete decoder;
    decoder = NULL;

    printf("delete copyRescale\n");
    delete copyRescale;
    copyRescale = NULL;

    printf("free_aligned\n");
    free_aligned(yuy2);
    free_aligned(yuy2_aux);

    printf("main end.\n");
    PlatformSignal::Reset();
    return 0;
}
