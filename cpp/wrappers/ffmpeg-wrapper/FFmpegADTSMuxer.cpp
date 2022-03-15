#include "FFmpegADTSMuxer.h"
namespace FFmpegWrapper {
#if USE_BITSTREAM_FILTER
    void FFmpegADTSMuxer::onDataFromFilter(AVPacket* pkt) {
        if (pkt->side_data_elems > 0) {
            //need to add ADTS Header here
            ARIBEIRO_ABORT((pkt->side_data_elems <= 0), "Bitstream Filter without extradata\n");
            //if (pkt->side_data_elems <= 0){
            //    printf("Bitstream Filter without extradata\n");
            //    exit(-1);
            //}
            AVPacketSideData *sideData = &pkt->side_data[0];
            ARIBEIRO_ABORT((sideData->type != AV_PKT_DATA_NEW_EXTRADATA), "Wrong extradata\n");
            //if (sideData->type != AV_PKT_DATA_NEW_EXTRADATA){
            //    printf("Wrong extradata\n");
            //    exit(-1);
            //}
            ARIBEIRO_ABORT((sideData->size < 2), "Missing extradata parameters\n");
            //if (sideData->size < 2){
            //    printf("Missing extradata parameters\n");
            //    exit(-1);
            //}
            memset(FakeADTSHeader, 0, sizeof(uint8_t) * 7);
            FakeADTSHeader[0] = 0xff;
            FakeADTSHeader[1] = 0xf1;
            //{0xff,0xf1,0,0,0,0,0};

            // bytes: [11111000]
            uint8_t objectType = (sideData->data[0] >> 3) - 1;//2 bits
            // bytes: [00000111] [10000000]
            uint8_t samplingIndex = ((sideData->data[0] & 0x07) << 1) | (sideData->data[1] >> 7);//4bits
            // bytes: [00000000] [01111000]
            uint8_t channelConfig = (sideData->data[1] >> 3) & 0xf; // 3 bits

            //16 - 17 [00000000][0000000][11000000]
            FakeADTSHeader[2] = objectType << 6;
            //bits 18 - 21 [00000000][0000000][00111100]
            FakeADTSHeader[2] |= samplingIndex << 2;
            //bits 23 - 25 [00000000][0000000][00000001][11000000]
            FakeADTSHeader[2] |= channelConfig >> 2;
            FakeADTSHeader[3] = channelConfig << 6;

        }

        uint32_t framelength = (pkt->size + 7) & 0x1fff;//13 bits
        ARIBEIRO_ABORT((framelength != (pkt->size + 7)), "wrong framelength\n");
        //if (framelength != (pkt->size + 7 )){
        //    printf("wrong framelength\n");
        //    exit(-1);
        //}
        //bits 30 - 42 [00000000][0000000][00000000][00000011][11111111][11100000]
        FakeADTSHeader[3] &= ~0x03;
        FakeADTSHeader[3] |= framelength >> 11;
        FakeADTSHeader[4] = (framelength >> 3) & 0xff;
        FakeADTSHeader[5] = (framelength << 5) & 0xff;

        //printf(" [FFmpegADTSMuxer] framelength size calculated: %u (%i)\n", framelength-7, size - (framelength-7) );

        OnData(FakeADTSHeader, 7);
        OnData(pkt->data, pkt->size);
    }
#endif

    int FFmpegADTSMuxer::onDataFromMuxer(void *userData, uint8_t *data, int size) {
        FFmpegADTSMuxer *muxer = (FFmpegADTSMuxer*)userData;

        ARIBEIRO_ABORT(size <= 7, "pkt size too small: %i\n", size);
        //if (size<=7){
        //    printf("[FFmpegADTSMuxer] pkt size too small: %i\n",size);
        //    exit(-1);
        //}
        //bits 54 - 55
        uint32_t num_frames = (data[6] & 0x03) + 1;//readbits(54,2,data,size)+1;

        ARIBEIRO_ABORT(num_frames != 1, "NumFrames > 1 not implemented: %u\n", num_frames);
        //if (num_frames != 1){
        //    printf("[FFmpegADTSMuxer] NumFrames > 1 not implemented: %u\n",num_frames);
        //    exit(-1);
        //}

#if USE_BITSTREAM_FILTER
        FFmpegBitstreamFilterADTStoASC &filter = muxer->filter;
        filter.sendBuffer(data, size);
#else
        muxer->OnData(data, size);
#endif
        return size;
    }

    FFmpegADTSMuxer::FFmpegADTSMuxer() {

        initialized = false;

        adts_container = NULL;
        adts_container_ctx = NULL;
        adts_container_buffer = NULL;
        adts_container_buffer_size = 4096;

        adts_avio_ctx = NULL;
        adts_stream = NULL;

#if USE_BITSTREAM_FILTER
        memset(FakeADTSHeader, 0, sizeof(FakeADTSHeader));
#endif
    }

    FFmpegADTSMuxer::~FFmpegADTSMuxer() {

        if (adts_container_ctx != NULL)
            avformat_free_context(adts_container_ctx);
        if (adts_container_buffer != NULL)
            av_freep(&adts_container_buffer);
        if (adts_avio_ctx != NULL)
            av_freep(&adts_avio_ctx);

        adts_container = NULL;
        adts_container_ctx = NULL;
        adts_container_buffer = NULL;
        adts_avio_ctx = NULL;
        adts_stream = NULL;
    }


    void FFmpegADTSMuxer::init(AVCodecContext* audio_encoder_ctx, const FFmpeg_OnDataMethodPtrT &OnData) {

        if (initialized)
            return;

        globalInitFFMPEG();

        initialized = true;

        encoded_pkt_counter = 1;

        this->OnData = OnData;
        this->audio_encoder_ctx = audio_encoder_ctx;

        /*
            * Create the ADTS container for the encoded frames
            */
        adts_container = av_guess_format("adts", NULL, NULL);

        int ret_val;

        ARIBEIRO_ABORT(!adts_container, "Could not find adts output format\n");
        //if (!adts_container) {
        //    fprintf(stderr, "Could not find adts output format\n");
        //    exit(1);
        //}
        ARIBEIRO_ABORT(
            ((ret_val = avformat_alloc_output_context2(&adts_container_ctx, adts_container, "", NULL)) < 0),
            "Could not create output context (error '%s')\n", av_err2str(ret_val));

        //if ((ret_val = avformat_alloc_output_context2(&adts_container_ctx, adts_container, "", NULL)) < 0) {
        //    fprintf(stderr, "Could not create output context (error '%s')\n", av_err2str(ret_val));
        //    exit(1);
        //}

        ARIBEIRO_ABORT(
            (!(adts_container_buffer = (uint8_t*)av_malloc(adts_container_buffer_size))),
            "Could not allocate a buffer for the I/O output context\n");

        //if (!(adts_container_buffer = (uint8_t*)av_malloc(adts_container_buffer_size))) {
        //    fprintf(stderr, "Could not allocate a buffer for the I/O output context\n");
        //    exit(1);
        //}
        // Create an I/O context for the adts container with a write callback (write_adts_muxed_data()), so that muxed data will be accessed through this function.
        ARIBEIRO_ABORT(
            (!(adts_avio_ctx = avio_alloc_context(adts_container_buffer,
                adts_container_buffer_size,
                1,
                this,
                NULL,
                &FFmpegADTSMuxer::onDataFromMuxer,
                NULL))),
            "Could not create I/O output context\n");

        //if (!(adts_avio_ctx = avio_alloc_context(adts_container_buffer,
        //                                            adts_container_buffer_size,
        //                                            1,
        //                                            this,
        //                                            NULL ,
        //                                            &FFmpegADTSMuxer::onDataFromMuxer,
        //                                            NULL))) {
        //    fprintf(stderr, "Could not create I/O output context\n");
        //    exit(1);
        //}
        // Link the container's context to the previous I/O context
        adts_container_ctx->pb = adts_avio_ctx;

        ARIBEIRO_ABORT(
            (!(adts_stream = avformat_new_stream(adts_container_ctx, NULL))),
            "Could not create new stream\n");
        //if (!(adts_stream = avformat_new_stream(adts_container_ctx, NULL))) {
        //    fprintf(stderr, "Could not create new stream\n");
        //    exit(1);
        //}

        adts_stream->id = adts_container_ctx->nb_streams - 1;
        // Copy the encoder's parameters
        avcodec_parameters_from_context(adts_stream->codecpar, audio_encoder_ctx);
        // Allocate the stream private data and write the stream header
        ARIBEIRO_ABORT(
            (avformat_write_header(adts_container_ctx, NULL) < 0),
            "avformat_write_header() error\n");

        //if (avformat_write_header(adts_container_ctx, NULL) < 0) {
        //    fprintf(stderr, "avformat_write_header() error\n");
        //    exit(1);
        //}

        //filter.initFromStream(adts_stream);
#if USE_BITSTREAM_FILTER
        filter.initFromCodecContext(audio_encoder_ctx, FFmpeg_OnAVPacketMethodPtrT(this, &FFmpegADTSMuxer::onDataFromFilter));
#endif
    }

    //need to link this method to the OnData from the AudioEncoder
    void FFmpegADTSMuxer::writeData(AVFrame *input_frame, AVPacket *encoded_packet) {

        if (!initialized)
            return;

        curr_pts = input_frame->nb_samples * (encoded_pkt_counter - 1);
        encoded_packet->pts = encoded_packet->dts = curr_pts;

        int ret_val;
        ARIBEIRO_ABORT(
            ((ret_val = av_write_frame(adts_container_ctx, encoded_packet)) < 0),
            "Error calling av_write_frame() (error '%s')\n", av_err2str(ret_val));

        //if ((ret_val = av_write_frame(adts_container_ctx, encoded_packet)) < 0) { //(e)
        //    fprintf(stderr, "Error calling av_write_frame() (error '%s')\n", av_err2str(ret_val));
        //    exit(1);
        //}

        /*
        fprintf(stderr, "Encoded AAC packet %d, size=%d, pts_time=%s\n",
                encoded_pkt_counter,
                encoded_packet->size,
                av_ts2timestr(encoded_packet->pts, &audio_encoder_ctx->time_base));
        */
        encoded_pkt_counter++;
    }


    void FFmpegADTSMuxer::endStream() {
        if (adts_container_ctx == NULL)
            return;
        av_write_trailer(adts_container_ctx);
    }
}
