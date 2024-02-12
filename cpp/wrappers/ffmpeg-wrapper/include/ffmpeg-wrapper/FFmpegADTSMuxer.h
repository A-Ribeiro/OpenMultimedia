#pragma once

#include "FFmpegCommon.h"
#include "FFmpegBitstreamFilterADTStoASC.h"

namespace FFmpegWrapper {

#define USE_BITSTREAM_FILTER 1

    /// \brief Wrapper class to the ffmpeg ADTS container format muxer
    ///
    /// This class handles the ffmpeg function call sequence to do the muxing of ADTS format.
    ///
    /// This class was made to integrate with the #FFmpegAudioEncoder class.
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
    /// FFmpegADTSMuxer adtsMuxer;
    /// FFmpegAudioEncoder audioEncoder;
    ///
    /// void OnDataFromMuxer(const uint8_t *data, size_t size){
    ///     fwrite(data, sizeof(uint8_t), (size_t)size, outputAAC);
    /// }
    ///
    /// audioEncoder.initAAC(44100,//sample rate
    ///     2,//channels
    ///     192000,//bitrate
    ///     EventCore::Callback<void(AVFrame *input_frame, AVPacket *encoded_packet)>(&adtsMuxer, &FFmpegADTSMuxer::writeData));
    ///
    /// adtsMuxer.init(audioEncoder.getCtx(), &OnDataFromMuxer);
    ///
    /// // You can write any audio data to the audio encoder
    /// // The encoder will feed the #FFmpegADTSMuxer instance
    /// audioEncoder.writeBufferFloat((float*)flt_buffer_from_audio_card, frames);
    ///
    /// ...
    ///
    /// audioEncoder.flushQueuedFrames();
    /// adtsMuxer.endStream();
    /// \endcode
    ///
    /// \author Alessandro Ribeiro
    ///
    class FFmpegADTSMuxer: public EventCore::HandleCallback {

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(59, 0, 100)
        AVOutputFormat *adts_container;
#else
        const AVOutputFormat *adts_container;
#endif


        AVFormatContext *adts_container_ctx;
        uint8_t *adts_container_buffer;
        size_t adts_container_buffer_size;

        //callback registration
        AVIOContext *adts_avio_ctx;
        AVStream *adts_stream;

        int64_t curr_pts;
        int encoded_pkt_counter;

        EventCore::Callback<void(const uint8_t *, size_t)> OnData;

        //external ref:
        AVCodecContext* audio_encoder_ctx;

        bool initialized;

#if USE_BITSTREAM_FILTER
        FFmpegBitstreamFilterADTStoASC filter;
        uint8_t FakeADTSHeader[7];

        void onDataFromFilter(AVPacket* pkt);
#endif

        static int onDataFromMuxer(void *userData, uint8_t *data, int size);

    public:

        FFmpegADTSMuxer();
        virtual ~FFmpegADTSMuxer();

        /// \brief Initialize the container
        ///
        /// It uses the audio encoder parameters to write the header correctly.
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
        /// FFmpegADTSMuxer adtsMuxer;
        /// FFmpegAudioEncoder audioEncoder;
        ///
        /// void OnDataFromMuxer(const uint8_t *data, size_t size){
        ///     ...
        /// }
        ///
        /// ...
        ///
        /// adtsMuxer.init(audioEncoder.getCtx(), &OnDataFromMuxer);
        ///
        /// \endcode
        ///
        /// \author Alessandro Ribeiro
        /// \param audio_encoder_ctx The internal ffmpeg audio encoder context
        /// \param OnData encoded data writing callback.
        ///
        void init(AVCodecContext* audio_encoder_ctx, const EventCore::Callback<void(const uint8_t *, size_t)> &OnData);

        //need to link this method to the OnData from the AudioEncoder

        /// \brief This container encoded expects this method to be called when a new audio encoded data is available
        ///
        /// It has the same signature the #EventCore::Callback<void(AVFrame *input_frame, AVPacket *encoded_packet)> defines.
        ///
        /// It can connect directly to the output of the #FFmpegAudioEncoder.
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
        /// FFmpegADTSMuxer adtsMuxer;
        /// FFmpegAudioEncoder audioEncoder;
        ///
        /// audioEncoder.initAAC(44100,//sample rate
        ///     2,//channels
        ///     192000,//bitrate
        ///     EventCore::Callback<void(AVFrame *input_frame, AVPacket *encoded_packet)>(&adtsMuxer, &FFmpegADTSMuxer::writeData));
        ///
        /// ...
        ///
        /// \endcode
        ///
        /// \author Alessandro Ribeiro
        /// \param input_frame Uses the nb_samples to compute the PTS (Presentation timestamp)
        /// \param encoded_packet Contains the ffmpeg audio encoded packet.
        ///
        void writeData(AVFrame *input_frame, AVPacket *encoded_packet);

        /// \brief Flush the ramainder data in the ffmpeg internal buffers.
        ///
        /// Need to call this when reachs the end of the file or the stream.
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
        /// FFmpegADTSMuxer adtsMuxer;
        /// FFmpegAudioEncoder audioEncoder;
        ///
        /// ...
        ///
        /// audioEncoder.flushQueuedFrames();
        /// adtsMuxer.endStream();
        ///
        /// \endcode
        ///
        /// \author Alessandro Ribeiro
        ///
        void endStream();
    };
}
