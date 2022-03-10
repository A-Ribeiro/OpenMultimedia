
#include "FFmpegCommon.h"

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
        //#if defined(OS_TARGET_linux)
        static bool initialized = false;
        if (!initialized) {
            initialized = true;
            av_register_all();
            avcodec_register_all();

            av_log_set_level(AV_LOG_WARNING);
        }
        //#endif
    }
}