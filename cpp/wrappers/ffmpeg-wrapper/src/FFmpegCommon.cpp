
#include <ffmpeg-wrapper/FFmpegCommon.h>

extern "C" {
    #include <libavcodec/avcodec.h>
    //#include <libavutil/log.h>
    #include <libavformat/avformat.h>
    //#include <libavutil/timestamp.h>
    //#include <libswresample/swresample.h>
    #include <libavutil/pixdesc.h>

}
namespace FFmpegWrapper {
    void globalInitFFMPEG() {
        //#if defined(__linux__)
        static bool initialized = false;
        if (!initialized) {
            initialized = true;

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 9, 100)
            av_register_all();
#endif

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 10, 100)
            avcodec_register_all();
#endif

            av_log_set_level(AV_LOG_WARNING);
        }
        //#endif
    }
}