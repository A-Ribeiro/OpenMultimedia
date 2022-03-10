#ifndef FFmpegAudioEncoder__H
#define FFmpegAudioEncoder__H

#include "FFmpegCommon.h"
#include "FFmpegAudioFrameConverter.h"

namespace FFmpegWrapper {
    /// \brief Wrapper class to the ffmpeg audio encoder
    ///
    /// This class handles the ffmpeg function call sequence to do the encoding of audio streams.
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
    /// FFmpegAudioEncoder audioEncoder;
    ///
    /// void OnDataFromEncoder(AVFrame *input_frame, AVPacket *encoded_packet){
    ///     fwrite(encoded_packet->data, sizeof(uint8_t), (size_t)encoded_packet->size, outputAAC);
    /// }
    ///
    /// audioEncoder.initAAC(44100,//sample rate
    ///     2,//channels
    ///     192000,//bitrate
    ///     &OnDataFromEncoder);
    ///
    /// // You can write any audio data to the audio encoder
    /// audioEncoder.writeBufferFloat((float*)flt_buffer_from_audio_card, frames);
    ///
    /// ...
    ///
    /// audioEncoder.flushQueuedFrames();
    /// \endcode
    ///
    /// \author Alessandro Ribeiro
    ///
    class FFmpegAudioEncoder {

        bool initialized;

        AVPacket *pkt;
        AVFrame *frame;
        AVCodecContext  *ctx;
        AVCodec *codec;

        //needs this to convert from float to float-planar to aac encoding
        FFmpegAudioFrameConverter float2floatPlanarFrameConverter;

        FFmpeg_OnAVFrameAVPacketMethodPtrT OnData;

    public:
        AVCodecContext* getCtx();///< gets the ffmpeg internal context configuration
        FFmpegAudioEncoder();
        virtual ~FFmpegAudioEncoder();

        /*
        Common AAC configurations:
         512kbps, 384kbps, 320kbps, 6, 160kbps
         64kbps(quality:5), 128kbps(quality:6,5), 192kbps(quality:8),
         256kbps(quality:10), 320kbps(quality:10)

        Parameters example:
            sampleRate = 44100
            channels = 2
            audioBitrate = 192000
         */
         /// \brief Initialize this context configuring the AAC enconding type
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
         /// FFmpegAudioEncoder audioEncoder;
         ///
         /// void OnDataFromEncoder(AVFrame *input_frame, AVPacket *encoded_packet){
         ///     ...
         /// }
         ///
         /// audioEncoder.initAAC(44100,//sample rate
         ///     2,//channels
         ///     192000,//bitrate
         ///     &OnDataFromEncoder);
         ///
         /// ...
         ///
         /// audioEncoder.flushQueuedFrames();
         /// \endcode
         ///
         /// \author Alessandro Ribeiro
         /// \param sampleRate Audio input samplerate: Example: 44100
         /// \param channels Audio input channels: Example: 2
         /// \param audioBitrate The audio bitrate. Example: 192000
         /// \param OnData encoded data writing callback.
         ///
        void initAAC(int sampleRate, int channels, int audioBitrate,
            const FFmpeg_OnAVFrameAVPacketMethodPtrT &OnData);

        /// \brief Get the total amount of samples that is needed to write to this encoder
        ///
        /// The total samples = number_of_frames * number_of_channels
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
        /// FFmpegAudioEncoder audioEncoder;
        ///
        /// ...
        ///
        /// int total_samples = audioEncoder.getTotalSamples();
        /// float input_buffer = new float[total_samples];
        ///
        /// ...
        /// \endcode
        ///
        /// \author Alessandro Ribeiro
        ///
        int getTotalSamples();

        /// \brief Write audio data to the encoder
        ///
        /// This can be called with the audio card buffer.
        ///
        /// One float for each sample. If you have 2 channels, so each frame will need 2 floats in the buffer.
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
        /// FFmpegAudioEncoder audioEncoder;
        ///
        /// ...
        ///
        /// float flt_buffer_from_audio_card = new float[frames * channels];
        ///
        /// ...
        ///
        /// audioEncoder.writeBufferFloat((float*)flt_buffer_from_audio_card, frames);
        /// \endcode
        ///
        /// \author Alessandro Ribeiro
        /// \param data Audio data (one float per channel) (all channels per one frame)
        /// \param frameCount Number of frames
        ///
        void writeBufferFloat(const float *data, int frameCount);


        /// \brief Empty the internal input ffmpeg queue buffers
        ///
        /// Use this when you reach the end of the stream.
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
        /// FFmpegAudioEncoder audioEncoder;
        ///
        /// ...
        ///
        /// audioEncoder.flushQueuedFrames();
        /// \endcode
        ///
        /// \author Alessandro Ribeiro
        ///
        void flushQueuedFrames();

    };
}

#endif