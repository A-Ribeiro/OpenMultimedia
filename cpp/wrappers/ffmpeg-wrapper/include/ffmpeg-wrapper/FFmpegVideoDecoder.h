#pragma once

#include "FFmpegCommon.h"
#include "H264Utils.h"

namespace FFmpegWrapper {

    enum VideoType {
        VideoType_NONE = 0,
        VideoType_HEVC = 1,
        VideoType_H264 = 2,
        VideoType_3GPP = 3,
        VideoType_MJPEG = 4
    };

    struct VideoData_YUV420P {
        int width;
        int height;
        Platform::ObjectBuffer buffer;
    };

    class FFmpegVideoDecoder: public EventCore::HandleCallback {

        AVCodecID codecID;

        AVPacket *pkt;
        AVFrame *frame;
        AVFrame *converted_frame;
        AVCodecContext  *ctx;
        const AVCodec *codec;
        AVCodecParserContext * parser;

        struct SwsContext *convert_to_420P_ctx;
        uint8_t* convert_to_420P_buffer_data;

        //ObjectBuffer buffer;

        Platform::ObjectPool<VideoData_YUV420P> dataPool;
        Platform::ObjectQueue<VideoData_YUV420P*> dataQueue;

        Platform::Thread *onDataThread;

        Platform::Mutex mutex;


        void OnData_Thread();


        std::vector<std::string> listDecoders();

        void sendSimpleBlockData(const uint8_t* input_data, int size);

    public:

        VideoType videoType;

        EventCore::Callback<void(int width, int height, const uint8_t *data, size_t size)> onData;

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
