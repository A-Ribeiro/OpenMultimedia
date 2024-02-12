#include <ffmpeg-wrapper/FFmpegAudioFrameConverter.h>

namespace FFmpegWrapper {
    FFmpegAudioFrameConverter::FFmpegAudioFrameConverter() {
        initialized = false;
        audio_convert_context = NULL;
        converted_audio_frame = NULL;
    }

    FFmpegAudioFrameConverter::~FFmpegAudioFrameConverter() {
        if (converted_audio_frame != NULL)
            av_frame_free(&converted_audio_frame);
        if (audio_convert_context != NULL)
            swr_free(&audio_convert_context);
        converted_audio_frame = NULL;
        audio_convert_context = NULL;
    }

    //AVSampleFormat targetFmt = AV_SAMPLE_FMT_FLTP
    void FFmpegAudioFrameConverter::init(AVCodecContext *audio_encoder_ctx,
        AVSampleFormat input_sample_fmt,
        int input_sample_rate) {
        if (initialized)
            return;
        initialized = true;
        globalInitFFMPEG();

#if LIBSWRESAMPLE_VERSION_INT < AV_VERSION_INT(4, 5, 100)
        
        audio_convert_context =
            swr_alloc_set_opts(NULL,
                //
                // target params
                //
                av_get_default_channel_layout(audio_encoder_ctx->channels),
                audio_encoder_ctx->sample_fmt,
                audio_encoder_ctx->sample_rate,
                //
                // src params
                //
                av_get_default_channel_layout(audio_encoder_ctx->channels),
                input_sample_fmt,
                input_sample_rate,
                0, NULL);
#else
        AVChannelLayout ch_layout_aux;
        av_channel_layout_default(&ch_layout_aux,audio_encoder_ctx->ch_layout.nb_channels);

        int error =
            swr_alloc_set_opts2(&audio_convert_context,
                //
                // target params
                //
                &ch_layout_aux,
                audio_encoder_ctx->sample_fmt,
                audio_encoder_ctx->sample_rate,
                //
                // src params
                //
                &ch_layout_aux,
                input_sample_fmt,
                input_sample_rate,
                0, NULL);
        ITK_ABORT(error < 0, "could not allocate ressample context.");
#endif



        ITK_ABORT(
            !audio_convert_context,
            "Could not allocate resample context\n");
        //if (!audio_convert_context) {
        //    fprintf(stderr, "Could not allocate resample context\n");
        //    exit(1);
        //}

        int ret_val = 0;
        //if ((ret_val = swr_init(audio_convert_context)) < 0) {
            //fprintf(stderr, "Could not open resample context\n");
            //exit(1);
        //}

        ITK_ABORT(
            (!(converted_audio_frame = av_frame_alloc())),
            "Could not allocate resampled frame\n");
        //if (!(converted_audio_frame = av_frame_alloc())) {
        //    fprintf(stderr, "Could not allocate resampled frame\n");
        //    exit(1);
        //}

        converted_audio_frame->nb_samples = audio_encoder_ctx->frame_size;
        converted_audio_frame->format = audio_encoder_ctx->sample_fmt;

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(59, 24, 100)
        converted_audio_frame->channels = audio_encoder_ctx->channels;
        converted_audio_frame->channel_layout = audio_encoder_ctx->channel_layout;
#else
        converted_audio_frame->ch_layout = audio_encoder_ctx->ch_layout;
#endif
      
        converted_audio_frame->sample_rate = audio_encoder_ctx->sample_rate;

        ITK_ABORT(
            ((ret_val = av_frame_get_buffer(converted_audio_frame, 0)) < 0),
            "Could not allocate a buffer for resampled frame samples (error '%s')\n", av_err2str(ret_val));

        //if ((ret_val = av_frame_get_buffer(converted_audio_frame, 0)) < 0) {
        //    fprintf(stderr, "Could not allocate a buffer for resampled frame samples (error '%s')\n", av_err2str(ret_val));
        //    exit(1);
        //}

    }

    AVFrame * FFmpegAudioFrameConverter::convert(AVFrame *frame) {
        if (!initialized)
            return frame;

        swr_convert_frame(audio_convert_context, converted_audio_frame, (const AVFrame*)frame);

        return converted_audio_frame;
    }

}