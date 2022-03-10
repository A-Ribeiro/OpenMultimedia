#include "FFmpegBitstreamFilterADTStoASC.h"
namespace FFmpegWrapper {
    void FFmpegBitstreamFilterADTStoASC::retrievePKT() {
        int ret = 0;
        while (ret >= 0) {
            ret = av_bsf_receive_packet(ctx, resultpkt);
            if (ret == AVERROR(EAGAIN)) {
                //no frame available yet.... need to feed more frames to get any output
                break;
            }
            else if (ret == AVERROR_EOF) {
                break;
                //fprintf(stderr, "receive packet EOF\n");
                //exit(1);
                //return;
            }
            else if (ret < 0) {
                ARIBEIRO_ABORT(
                    true,
                    "error to use filter adtstoasc...\n");
                //fprintf(stderr,"error to use filter adtstoasc...\n");
                //exit(1);
            }

            // pkt->data : bin data
            // pkt->size : bin size bytes
            OnData(resultpkt);
            av_packet_unref(resultpkt);
        }
    }

    FFmpegBitstreamFilterADTStoASC::FFmpegBitstreamFilterADTStoASC() {
        globalInitFFMPEG();

        bsf = av_bsf_get_by_name("aac_adtstoasc");
        ctx = NULL;
        initialized = false;
        resultpkt = NULL;
    }
    FFmpegBitstreamFilterADTStoASC::~FFmpegBitstreamFilterADTStoASC() {
        if (ctx != NULL)
            av_bsf_free(&ctx);
        if (resultpkt != NULL)
            av_packet_free(&resultpkt);
        ctx = NULL;
        resultpkt = NULL;
    }

    void FFmpegBitstreamFilterADTStoASC::initFromStream(AVStream*stream, const FFmpeg_OnAVPacketMethodPtrT &OnData) {
        if (initialized)
            return;

        initialized = true;
        this->OnData = OnData;

        resultpkt = av_packet_alloc();

        ARIBEIRO_ABORT(
            !resultpkt,
            "could not allocate the packet\n");

        //if (!resultpkt) {
        //    fprintf(stderr, "could not allocate the packet\n");
        //    exit(1);
        //}

        int ret = av_bsf_alloc(bsf, &ctx);

        ARIBEIRO_ABORT(
            ret < 0,
            "error to allocate AACToASCBitstreamFilter\n");
        //if (ret < 0){
        //    fprintf(stderr,"error to allocate AACToASCBitstreamFilter\n");
        //    exit(-1);
        //}

        //input config
        ret = avcodec_parameters_copy(ctx->par_in, stream->codecpar);
        ARIBEIRO_ABORT(
            ret < 0,
            "error to copy codec parameter\n");
        //if (ret < 0){
        //    fprintf(stderr,"error to copy codec parameter\n");
        //    exit(-1);
        //}
        ctx->time_base_in = stream->time_base;

        ret = av_bsf_init(ctx);
        ARIBEIRO_ABORT(
            ret < 0,
            "error on av_bsf_init\n");
        //if (ret < 0){
        //    fprintf(stderr,"error on av_bsf_init\n");
        //    exit(-1);
        //}

        /*
        //output config
        ret = avcodec_parameters_copy(stream->codecpar, ctx->par_out);
        if (ret < 0){
            fprintf(stderr,"error on avcodec_parameters_copy\n");
            exit(-1);
        }
        stream->time_base = ctx->time_base_out;
        */
    }

    void FFmpegBitstreamFilterADTStoASC::initFromCodecContext(const AVCodecContext *codecCTX, const FFmpeg_OnAVPacketMethodPtrT &OnData) {

        if (initialized)
            return;

        initialized = true;
        this->OnData = OnData;

        resultpkt = av_packet_alloc();
        ARIBEIRO_ABORT(
            !resultpkt,
            "could not allocate the packet\n");
        //if (!resultpkt) {
        //    fprintf(stderr, "could not allocate the packet\n");
        //    exit(1);
        //}

        int ret = av_bsf_alloc(bsf, &ctx);
        ARIBEIRO_ABORT(
            ret < 0,
            "error to allocate AACToASCBitstreamFilter\n");
        //if (ret < 0){
        //    fprintf(stderr,"error to allocate AACToASCBitstreamFilter\n");
        //    exit(-1);
        //}
        ctx->time_base_in = codecCTX->time_base;

        ret = avcodec_parameters_from_context(ctx->par_in, codecCTX);
        ARIBEIRO_ABORT(
            ret < 0,
            "error to copy codec parameter\n");
        //if (ret < 0){
        //    fprintf(stderr,"error to copy codec parameter\n");
        //    exit(-1);
        //}

        ret = av_bsf_init(ctx);
        ARIBEIRO_ABORT(
            ret < 0,
            "error on av_bsf_init\n");
        //if (ret < 0){
        //    fprintf(stderr,"error on av_bsf_init\n");
        //    exit(-1);
        //}


        //ret = avcodec_parameters_copy(out->codecpar, ctx->par_out);
        //if (ret < 0)
        //goto fail;
        //out->time_base = ctx->time_base_out;
    }

    void FFmpegBitstreamFilterADTStoASC::sendAVPacket(AVPacket *pkt) {
        if (!initialized)
            return;

        int ret = av_bsf_send_packet(ctx, pkt);
        ARIBEIRO_ABORT(
            ret < 0,
            "error on av_bsf_send_packet\n");
        //if (ret < 0){
        //    fprintf(stderr,"error on av_bsf_send_packet\n");
        //    exit(-1);
        //}

        retrievePKT();
    }

    void FFmpegBitstreamFilterADTStoASC::sendBuffer(void *buffer, size_t size) {
        if (!initialized)
            return;

        AVPacket mpkt = { 0 };

        //memset(&mpkt,sizeof(AVPacket),0);
        mpkt.data = (uint8_t*)buffer;
        mpkt.size = size;

        int ret = av_bsf_send_packet(ctx, &mpkt);

        ARIBEIRO_ABORT(
            ret < 0,
            "error on av_bsf_send_packet\n");
        //if (ret < 0){
        //    fprintf(stderr,"error on av_bsf_send_packet\n");
        //    exit(-1);
        //}

        retrievePKT();
    }
}