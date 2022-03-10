#ifndef FFmpegAudioFrameConverter__H
#define FFmpegAudioFrameConverter__H

#include "FFmpegCommon.h"

//needs this class to convert from 
//  float to float-planar
//input for the AAC encoder
namespace FFmpegWrapper {
    /// \brief Wrapper class to the ffmpeg SWR audio converting routines
    ///
    /// With this class it is possible to convert any audio input<br />
    /// samplerate to the samplerate and format an encoder expects as input.
    ///
    /// Example:
    ///
    /// \code
    /// #include <aribeiro/aribeiro.h>
    /// #include <ffmpeg-wrapper/ffmpeg-wrapper.h>
    /// using namespace aRibeiro;
    /// using namespace FFmpegWrapper;
    ///
    /// ...
    ///
    /// // AAC converting from float to float-planar
    /// FFmpegAudioFrameConverter float2floatPlanarFrameConverter;
    ///
    /// AVCodecContext  *context = avcodec_alloc_context3(avcodec_find_encoder(AV_CODEC_ID_AAC));
    /// context->sample_fmt = AV_SAMPLE_FMT_FLTP;
    /// context->sample_rate = 44100;
    ///
    /// ...
    ///
    /// AVFrame *frame = av_frame_alloc();
    /// frame->format = AV_SAMPLE_FMT_FLT;
    /// frame->sample_rate = context->sample_rate;
    ///
    /// ...
    ///
    /// // will convert the frame format AV_SAMPLE_FMT_FLT to
    /// // context format AV_SAMPLE_FMT_FLTP at the same samplerate
    /// float2floatPlanarFrameConverter.init(ctx,frame->format,frame->sample_rate);
    ///
    /// ...
    ///
    /// // Write some frame information
    /// av_frame_make_writable(frame);
    ///
    /// ...
    ///
    /// // convert the writen frame
    /// AVFrame* converted_frame = float2floatPlanarFrameConverter.convert(frame);
    ///
    /// // send the frame to the encoder
    /// avcodec_send_frame(ctx, converted_frame);
    ///
    /// ...
    /// \endcode
    ///
    /// \author Alessandro Ribeiro
    ///
    class FFmpegAudioFrameConverter {

        bool initialized;
        SwrContext *audio_convert_context;

    public:

        AVFrame *converted_audio_frame;///< Holds the converted frame data

        FFmpegAudioFrameConverter();

        virtual ~FFmpegAudioFrameConverter();

        //AVSampleFormat targetFmt = AV_SAMPLE_FMT_FLTP

        /// \brief Initialize this #FFmpegAudioFrameConverter
        ///
        /// It will convert the parameter input (input_sample_fmt and input_sample_rate) to match<br />
        /// the context format and sample rate.
        ///
        /// Example:
        ///
        /// \code
        /// #include <aribeiro/aribeiro.h>
        /// #include <ffmpeg-wrapper/ffmpeg-wrapper.h>
        /// using namespace aRibeiro;
        /// using namespace FFmpegWrapper;
        ///
        /// ...
        ///
        /// // AAC converting from float to float-planar
        /// FFmpegAudioFrameConverter float2floatPlanarFrameConverter;
        ///
        /// AVCodecContext  *context = avcodec_alloc_context3(avcodec_find_encoder(AV_CODEC_ID_AAC));
        /// context->sample_fmt = AV_SAMPLE_FMT_FLTP;
        /// context->sample_rate = 44100;
        ///
        /// ...
        ///
        /// AVFrame *frame = av_frame_alloc();
        /// frame->format = AV_SAMPLE_FMT_FLT;
        /// frame->sample_rate = context->sample_rate;
        ///
        /// ...
        ///
        /// // will convert the frame format AV_SAMPLE_FMT_FLT to
        /// // context format AV_SAMPLE_FMT_FLTP at the same samplerate
        /// float2floatPlanarFrameConverter.init(ctx,frame->format,frame->sample_rate);
        ///
        /// ...
        ///
        /// \endcode
        ///
        /// \author Alessandro Ribeiro
        /// \param audio_encoder_ctx Frame output configuration  (format and sample rate)
        /// \param input_sample_fmt Input format
        /// \param input_sample_rate Input sample rate
        ///
        void init(AVCodecContext *audio_encoder_ctx,
            AVSampleFormat input_sample_fmt = AV_SAMPLE_FMT_FLT,
            int input_sample_rate = 44100);


        /// \brief Converts one input frame to a context compatible frame
        ///
        /// Example:
        ///
        /// \code
        /// #include <aribeiro/aribeiro.h>
        /// #include <ffmpeg-wrapper/ffmpeg-wrapper.h>
        /// using namespace aRibeiro;
        /// using namespace FFmpegWrapper;
        ///
        /// ...
        ///
        /// // AAC converting from float to float-planar
        /// FFmpegAudioFrameConverter float2floatPlanarFrameConverter;
        /// AVFrame* frame;
        ///
        /// ...
        ///
        /// // Write some frame information
        /// av_frame_make_writable(frame);
        ///
        /// ...
        ///
        /// // convert the writen frame
        /// AVFrame* converted_frame = float2floatPlanarFrameConverter.convert(frame);
        ///
        /// // send the frame to the encoder
        /// avcodec_send_frame(ctx, converted_frame);
        ///
        /// ...
        /// \endcode
        ///
        /// \author Alessandro Ribeiro
        /// \param frame Input frame to convert
        /// \return converted frame compatible with the encoding context
        ///
        AVFrame * convert(AVFrame *frame);
    };
}


#endif
