#ifndef FFmpegVideoDecoder__H_
#define FFmpegVideoDecoder__H_

#include "FFmpegCommon.h"
#include "H264Utils.h"

namespace FFmpegWrapper {

    enum VideoType {
        VideoType_NONE = 0,
        VideoType_HEVC = 1,
        VideoType_H264 = 2,
        VideoType_3GPP = 3,
    };

    struct VideoData_YUV420P {
        int width;
        int height;
        ObjectBuffer buffer;
    };

    class FFmpegVideoDecoder {

        AVCodecID codecID;

        AVPacket *pkt;
        AVFrame *frame;
        AVFrame *converted_frame;
        AVCodecContext  *ctx;
        AVCodec *codec;
        AVCodecParserContext * parser;

        struct SwsContext *convert_to_420P_ctx;
        uint8_t* convert_to_420P_buffer_data;

        //ObjectBuffer buffer;

        ObjectPool<VideoData_YUV420P> dataPool;
        ObjectQueue<VideoData_YUV420P*> dataQueue;

        PlatformThread *onDataThread;

        void OnData_Thread();


        std::vector<std::string> listDecoders();

        void sendSimpleBlockData(const uint8_t* input_data, int size);

    public:

        VideoType videoType;

        FFmpeg_OnYUV420P_PtrT onData;

        ///< gets the ffmpeg internal context configuration
        AVCodecContext* getCtx();

        bool isInitialized();

        FFmpegVideoDecoder();
        void initialize(VideoType videoType);
        void release();
        void sendData(const uint8_t* input_data, int size);
        virtual ~FFmpegVideoDecoder();

    };

}

#endif