#ifndef FFmpegBitstreamFilterADTStoASC__H
#define FFmpegBitstreamFilterADTStoASC__H

#include "FFmpegCommon.h"

namespace FFmpegWrapper {
    /// \brief Wrapper class to the ffmpeg bitstream filter
    ///
    /// This bitstream filter converts the AAC/ADTS structure to raw AAC packets.
    ///
    /// There is a work around (kludge, MacGyver) inside the ffmpeg implementation that gets the<br />
    /// ADTS information and pass it as an AVPacketSideData* structure to the flv muxer.
    ///
    /// And this bitstream filter is responsible for that.
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
    /// FFmpegBitstreamFilterADTStoASC filter;
    /// FFmpegAudioEncoder audioEncoder;
    ///
    /// ...
    ///
    /// void onDataFromFilter( AVPacket* pkt ) {
    ///     ...
    /// }
    ///
    /// filter.initFromCodecContext(audioEncoder.getCtx(), &onDataFromFilter);
    ///
    /// ...
    ///
    /// // each data from the muxer can pass through the filter
    /// uint8_t *data;
    /// int size;
    /// filter.sendBuffer(data, size);
    ///
    /// \endcode
    ///
    /// \author Alessandro Ribeiro
    ///
    class FFmpegBitstreamFilterADTStoASC {

        const AVBitStreamFilter *bsf;
        AVBSFContext *ctx;
        AVPacket *resultpkt;

        FFmpeg_OnAVPacketMethodPtrT OnData;
        bool initialized;

        void retrievePKT();

    public:

        FFmpegBitstreamFilterADTStoASC();
        virtual ~FFmpegBitstreamFilterADTStoASC();

        /// \brief Initialize this bitstream filter from a ffmpeg stream definition
        ///
        /// \author Alessandro Ribeiro
        /// \param stream the ffmpeg stream that will be filtered
        /// \param OnData callback. Called when it has any data available
        ///
        void initFromStream(AVStream*stream, const FFmpeg_OnAVPacketMethodPtrT &OnData);

        /// \brief Initialize this bitstream filter from a ffmpeg codec context definition
        ///
        /// The context could be the ADTS muxer for example.
        ///
        /// \author Alessandro Ribeiro
        /// \param codecCTX the ffmpeg codec context that will be filtered
        /// \param OnData callback. Called when it has any data available
        ///
        void initFromCodecContext(const AVCodecContext *codecCTX, const FFmpeg_OnAVPacketMethodPtrT &OnData);

        /// \brief write a ffmpeg packet to this bitstream filter
        ///
        /// The result will be available through the callback registered in<br />
        /// any initialization method: #initFromStream, #initFromCodecContext
        ///
        /// \author Alessandro Ribeiro
        /// \param pkt Content containing the data to pass to the filter
        ///
        void sendAVPacket(AVPacket *pkt);

        /// \brief write a raw binary buffer to this bitstream filter
        ///
        /// The result will be available through the callback registered in<br />
        /// any initialization method: #initFromStream, #initFromCodecContext
        ///
        /// \author Alessandro Ribeiro
        /// \param buffer pointer to the buffer
        /// \param size the size of the buffer
        ///
        void sendBuffer(void *buffer, size_t size);
    };
}

#endif