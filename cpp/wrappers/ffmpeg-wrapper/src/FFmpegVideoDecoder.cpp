#include <ffmpeg-wrapper/FFmpegVideoDecoder.h>

namespace FFmpegWrapper
{

#define BLOCK_COUNT 16

    void FFmpegVideoDecoder::OnData_Thread()
    {
        bool isSignaled;
        while (!Platform::Thread::isCurrentThreadInterrupted())
        {
            VideoData_YUV420P *data = dataQueue.dequeue(&isSignaled);
            if (isSignaled)
                break;
            if (onData != nullptr)
                onData(data->width, data->height, data->buffer.data, data->buffer.size);
            dataPool.release(data);
        }
    }

    std::vector<std::string> FFmpegVideoDecoder::listDecoders()
    {
        globalInitFFMPEG();
        std::vector<std::string> result;
        printf("Checking decoders:\n");
        const AVCodec *codec = NULL;
        void *iter = NULL;
        // while ((codec = av_codec_next(codec)) != NULL) {
        while ((codec = av_codec_iterate(&iter)) != NULL)
        {
            if (codec->id == codecID && av_codec_is_decoder(codec))
            {
                printf("  ... %s\n", codec->name);
                result.push_back(codec->name);

                /*
                AVCodecContext  *ctx = avcodec_alloc_context3(codec);
                if (ctx != NULL) {//check can instantiate context
                    if (avcodec_open2(ctx, codec, NULL) >= 0) {
                        printf("  ... %s\n", codec->name);
                        result.push_back(codec->name);
                    }
                    avcodec_free_context(&ctx);
                }
                */
            }
        }
        printf("done.\n");
        return result;
    }

    AVCodecContext *FFmpegVideoDecoder::getCtx()
    {
        return ctx;
    }

    bool FFmpegVideoDecoder::isInitialized()
    {
        return ctx != NULL;
    }

    FFmpegVideoDecoder::FFmpegVideoDecoder()
    {

        videoType = VideoType_NONE;
        onData = NULL;
        pkt = NULL;
        frame = NULL;
        converted_frame = NULL;
        ctx = NULL;
        codec = NULL;
        parser = NULL;
        convert_to_420P_ctx = NULL;
        onDataThread = NULL;
        convert_to_420P_buffer_data = NULL;

        onDataThread = new Platform::Thread(EventCore::CallbackWrapper(
            &FFmpegVideoDecoder::OnData_Thread, this));
        onDataThread->start();
    }

    void FFmpegVideoDecoder::initialize(VideoType videoType)
    {

        Platform::AutoLock autoLock(&mutex);

        if (ctx != NULL)
            return;

        this->videoType = videoType;

        switch (videoType)
        {
        case VideoType_HEVC:
            codecID = AV_CODEC_ID_HEVC;
            break;
        case VideoType_H264:
            codecID = AV_CODEC_ID_H264;
            break;
        case VideoType_3GPP:
            codecID = AV_CODEC_ID_H263;
            break;
        case VideoType_MJPEG:
            codecID = AV_CODEC_ID_MJPEG;
            break;
        default:
            ITK_ABORT(true, "Error to set decoder type.\n");
            break;
        }

        std::vector<std::string> decoders = listDecoders();

        codec = NULL;
        // trying to open from the last to beggining of the codec list
        /*
        if (codec == NULL)
            for (int i = 0; codec == NULL && i < decoders.size(); i++) {
            //for (int i = decoders.size()-1; codec == NULL && i >= 0 ; i--) {
                codec = avcodec_find_decoder_by_name(decoders[i].c_str());
                ctx = avcodec_alloc_context3(codec);
                if (ctx == NULL) {
                    codec = NULL;
                }
            }
        */

        // h264_qsv, h264_cuvid ... depends on the hw_decoders list
        codec = avcodec_find_decoder(codecID);
        ITK_ABORT(codec == NULL, "decoder not found.\n");

        // codec->capabilities &= ~AV_CODEC_CAP_SUBFRAMES;

        ctx = avcodec_alloc_context3(codec);
        ITK_ABORT(ctx == NULL, "Error to create decoder context.\n");

        printf("Selected decoder: %s\n", ctx->codec->name);

        if (ctx->hwaccel != NULL)
            fprintf(stderr, "HW accel IN USE : %s\n", ctx->hwaccel->name);
        else
            fprintf(stderr, "NO HW accel IN USE\n");

        ctx->pix_fmt = AV_PIX_FMT_YUV420P;
        ctx->skip_frame = AVDISCARD_DEFAULT;
        ctx->error_concealment = FF_EC_GUESS_MVS | FF_EC_DEBLOCK; // | FF_EC_FAVOR_INTER;
        ctx->skip_loop_filter = AVDISCARD_DEFAULT;
        ctx->workaround_bugs = FF_BUG_AUTODETECT;
        ctx->codec_type = AVMEDIA_TYPE_VIDEO;
        ctx->codec_id = codecID;
        ctx->skip_idct = AVDISCARD_DEFAULT;
        ctx->err_recognition = AV_EF_CAREFUL;

        ctx->opaque = this;

        // ctx->flags2 |= AV_CODEC_FLAG2_CHUNKS;// | AV_CODEC_FLAG2_FAST;

        int rc = avcodec_open2(ctx, codec, NULL);
        ITK_ABORT(rc < 0, "avcodec_open2 error. %s\n", av_err2str(rc));

        pkt = av_packet_alloc();
        ITK_ABORT(!pkt, "could not allocate the packet\n");

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 133, 100)
        av_init_packet(pkt);
#else
#endif

        frame = av_frame_alloc();
        ITK_ABORT(frame == NULL, "Could not allocate video frame\n");

        converted_frame = av_frame_alloc();
        ITK_ABORT(converted_frame == NULL, "Could not allocate video frame\n");

        parser = av_parser_init(codecID);
        ITK_ABORT(parser == NULL, "av_parser_init error.\n");
    }

    void FFmpegVideoDecoder::release()
    {

        Platform::AutoLock autoLock(&mutex);

        // ITK_ABORT(onDataThread == PlatformThread::getCurrentThread(), "Trying to release this element from the child thread.\n");

        while (dataQueue.size() > 0)
        {
            printf("Data Queue Size: %i\n", dataQueue.size());
            dataPool.release(dataQueue.dequeue(NULL, true));
        }
        if (convert_to_420P_buffer_data != NULL)
        {
            av_free(convert_to_420P_buffer_data);
            convert_to_420P_buffer_data = NULL;
        }
        if (convert_to_420P_ctx != NULL)
        {
            sws_freeContext(convert_to_420P_ctx);
            convert_to_420P_ctx = NULL;
        }
        if (parser != NULL)
        {
            av_parser_close(parser);
            parser = NULL;
        }
        if (converted_frame != NULL)
        {
            converted_frame->width = 0;
            converted_frame->height = 0;
            av_frame_free(&converted_frame);
            converted_frame = NULL;
        }
        if (frame != NULL)
        {
            av_frame_free(&frame);
            frame = NULL;
        }
        if (pkt != NULL)
        {
            pkt->size = 0;
            pkt->data = NULL;
            av_packet_free(&pkt);
            pkt = NULL;
        }
        if (ctx != NULL)
        {
            avcodec_free_context(&ctx);
            ctx = NULL;
        }
        onData = NULL;
        codec = NULL;

        videoType = VideoType_NONE;
    }

    void FFmpegVideoDecoder::sendSimpleBlockData(const uint8_t *input_data, int input_size)
    {

        Platform::AutoLock autoLock(&mutex);

        pkt->size = input_size;
        pkt->data = (uint8_t *)input_data;
        int rc = 0;

        {
            rc = avcodec_send_packet(ctx, pkt);
            if (rc != AVERROR_INVALIDDATA && rc != AVERROR(EAGAIN))
                ITK_ABORT(rc < 0, "avcodec_send_packet error. %s\n", av_err2str(rc));
        }

        while (rc >= 0)
        {
            AVFrame *_frame = frame;
            rc = avcodec_receive_frame(ctx, _frame);
            if (rc == AVERROR(EAGAIN) || rc == AVERROR_EOF || rc == AVERROR_INPUT_CHANGED)
            {
                // no callback needed...
                break;
            }
            else if (rc < 0)
            {
                ITK_ABORT(true, "avcodec_receive_frame error. %s\n", av_err2str(rc));
            }

            if (ctx->pix_fmt != AV_PIX_FMT_YUV420P)
            {
                // need to convert to YUV420P

                if (convert_to_420P_ctx != NULL &&
                    (converted_frame->width != _frame->width || converted_frame->height != _frame->height))
                {
                    if (convert_to_420P_buffer_data != NULL)
                    {
                        av_free(convert_to_420P_buffer_data);
                        convert_to_420P_buffer_data = NULL;
                    }
                    if (convert_to_420P_ctx != NULL)
                    {
                        sws_freeContext(convert_to_420P_ctx);
                        convert_to_420P_ctx = NULL;
                    }
                }

                if (convert_to_420P_ctx == NULL)
                {
                    AVPixelFormat format = ctx->pix_fmt;

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(58, 134, 100)
                    if (format == AV_PIX_FMT_YUVJ420P)
                        format = AV_PIX_FMT_YUV420P;
                    else if (format == AV_PIX_FMT_YUVJ422P)
                        format = AV_PIX_FMT_YUV422P;
                    else if (format == AV_PIX_FMT_YUVJ444P)
                        format = AV_PIX_FMT_YUV444P;
#endif

                    convert_to_420P_ctx = sws_getContext(
                        // src
                        _frame->width, _frame->height, format,
                        // target
                        _frame->width, _frame->height, AV_PIX_FMT_YUV420P,
                        SWS_FAST_BILINEAR // SWS_BICUBIC
                        ,
                        NULL, NULL, NULL);

                    ITK_ABORT(convert_to_420P_ctx == NULL, "sws_getContext error.\n");

                    int buffer_size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, _frame->width, _frame->height, 32);
                    convert_to_420P_buffer_data = (uint8_t *)av_malloc(buffer_size);

                    ITK_ABORT(convert_to_420P_buffer_data == NULL, "creating convert_to_420P_buffer_data error.\n");

                    av_image_fill_arrays(converted_frame->data, converted_frame->linesize, convert_to_420P_buffer_data,
                                         AV_PIX_FMT_YUV420P, _frame->width, _frame->height, 32);

                    converted_frame->width = _frame->width;
                    converted_frame->height = _frame->height;
                    converted_frame->format = AV_PIX_FMT_YUV420P;
                }

                sws_scale(convert_to_420P_ctx,
                          _frame->data, _frame->linesize, 0, _frame->height,
                          converted_frame->data, converted_frame->linesize);

                _frame = converted_frame;
            }

            const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get((AVPixelFormat)_frame->format);

            int output_w = _frame->width;
            int output_h = _frame->height;
            int output_h_uv = output_h / 2;

            int yStride = output_w;
            int uvStride = yStride / 2;
            int ySize = yStride * output_h;
            int uvSize = uvStride * output_h_uv;

            int size = ySize + uvSize * 2;

            // avoid queue from increase size
            if (dataQueue.size() > 5)
            {
                printf("Queue full...\n");
                return;
            }

            VideoData_YUV420P *out = dataPool.create(true);

            out->width = output_w;
            out->height = output_h;

            out->buffer.setSize(size);

            // buffer.setSize(size);

            ITK_ABORT(((_frame->width >> desc->log2_chroma_w) != uvStride), "Wrong UV buffer width.\n");
            ITK_ABORT(((_frame->height >> desc->log2_chroma_h) != output_h_uv), "Wrong UV buffer height.\n");

            int frameStride_y = _frame->linesize[0];
            for (int h = 0; h < output_h; h++)
                memcpy(&out->buffer.data[h * yStride], &_frame->data[0][h * frameStride_y], output_w);

            int frameStride_u = _frame->linesize[1];
            for (int h = 0; h < output_h_uv; h++)
                memcpy(&out->buffer.data[ySize + h * uvStride], &_frame->data[1][h * frameStride_u], uvStride);

            int frameStride_v = _frame->linesize[2];
            for (int h = 0; h < output_h_uv; h++)
                memcpy(&out->buffer.data[ySize + uvSize + h * uvStride], &_frame->data[2][h * frameStride_v], uvStride);

            dataQueue.enqueue(out);

            // if (onData != NULL)
            //     onData(output_w, output_h, &buffer.data[0], size);
        }
    }

    void FFmpegVideoDecoder::sendData(const uint8_t *input_data, int input_data_size)
    {
        Platform::AutoLock autoLock(&mutex);

        // use parser to get just frame packets
        int pos_acc = 0;
        while (pos_acc < input_data_size)
        {

            uint8_t *_data = NULL;
            int _size = 0;
            int rc = av_parser_parse2(parser, ctx, &_data, &_size, &input_data[pos_acc], input_data_size - pos_acc, AV_NOPTS_VALUE, AV_NOPTS_VALUE, AV_NOPTS_VALUE);

            ITK_ABORT(rc < 0, "error whiling parse.\n");
            // rc is the parsed length
            pos_acc += rc;

            if (_size)
            {
                sendSimpleBlockData(_data, _size);
            }
        }
        // */

        /*
        int next_delimiter = 4;
        int index_start = 4;
        while (next_delimiter < input_data_size) {
            // find the next h264_demiliter
            next_delimiter = array_index_of(input_data, next_delimiter, input_data_size, h264_delimiter, 4);

            int delimiter_size = next_delimiter - index_start + 4;
            sendSimpleBlockData(&input_data[index_start-4], delimiter_size);

            //skip the h264_delimiter ( 0x 00 00 00 01 )
            next_delimiter += 4;
            index_start = next_delimiter;
        }
        // */
    }

    FFmpegVideoDecoder::~FFmpegVideoDecoder()
    {
        if (onDataThread != NULL)
        {
            printf("[FFmpegVideoDecoder] interrupt data thread start\n");
            onDataThread->interrupt();
            onDataThread->wait();
            delete onDataThread;
            onDataThread = NULL;
            printf("[FFmpegVideoDecoder] interrupt data thread done\n");
        }

        release();
    }

}