/// \file
#ifndef FFMPEG_INIT_H
#define FFMPEG_INIT_H

#ifndef __STDC_FORMAT_MACROS
    #define __STDC_FORMAT_MACROS
#endif

//https://stackoverflow.com/questions/2410459/encode-audio-to-aac-with-libavcodec
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/log.h>
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>
#include <libavutil/imgutils.h>
}

#if defined(OS_TARGET_linux) || defined(OS_TARGET_win)

    #undef av_err2str
    //#define av_err2str(errnum) av_make_error_string((char*)__builtin_alloca(AV_ERROR_MAX_STRING_SIZE), AV_ERROR_MAX_STRING_SIZE, errnum)

    av_always_inline char *av_err2str(int errnum) {
    static char str[AV_ERROR_MAX_STRING_SIZE];
    memset(str, 0, sizeof(str));
    return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
    }

    #undef av_ts2timestr
    //#define av_ts2timestr(ts, tb) av_ts_make_time_string((char[AV_TS_MAX_STRING_SIZE]){0}, ts, tb)

    av_always_inline char *av_ts2timestr(int64_t ts, AVRational *tb) {
    static char str[AV_TS_MAX_STRING_SIZE];
    memset(str, 0, sizeof(str));
    return av_ts_make_time_string(str, ts, tb);
    }

#endif

#include <aRibeiroCore/aRibeiroCore.h>
#include <aRibeiroPlatform/aRibeiroPlatform.h>
using namespace aRibeiro;

namespace FFmpegWrapper {
    /// \class FFmpeg_OnDataMethodPtrT
    /// \brief Callback pattern with data and size parameters
    ///
    /// Definition of a standard data/size method pointer pattern.
    ///
    /// Example of use with functions:
    ///
    /// \code
    ///    void callbackFunction(const uint8_t *data, size_t s){
    ///        ...
    ///    }
    ///
    ///    FFmpeg_OnDataMethodPtrT OnData;
    ///
    ///    OnData = &callbackFunction;
    ///
    ///    uint8_t *data;
    ///    size_t size;
    ///
    ///    ...
    ///
    ///    if (OnData != NULL)
    ///        OnData(data,size);
    /// \endcode
    ///
    /// Example of use with method:
    ///
    /// \code
    ///    class ExampleClass {
    ///    public:
    ///        void callbackFunction(const uint8_t *data, size_t s){
    ///            ...
    ///        }
    ///    };
    ///
    ///    ExampleClass obj;
    ///
    ///    FFmpeg_OnDataMethodPtrT OnData;
    ///
    ///    OnData = FFmpeg_OnDataMethodPtrT( &obj, &ExampleClass::callbackFunction );
    ///
    ///    uint8_t *data;
    ///    size_t size;
    ///
    ///    ...
    ///
    ///    if (OnData != NULL)
    ///        OnData(data,size);
    /// \endcode
    ///
    /// \author Alessandro Ribeiro
    ///
    DefineMethodPointer(FFmpeg_OnDataMethodPtrT, void, const uint8_t *data, size_t s) VoidMethodCall(data, s)

    DefineMethodPointer(FFmpeg_OnYUV420P_PtrT, void, int width, int height, const uint8_t *data, size_t size) VoidMethodCall(width, height, data, size)


        /// \class FFmpeg_OnAVPacketMethodPtrT
        /// \brief Callback pattern with ffmpeg AVPacket* parameter
        ///
        /// Used to define the bitstream filter for example.
        ///
        /// Example of use with functions:
        ///
        /// \code
        ///    void callbackFunction(AVPacket *pkt){
        ///        ...
        ///    }
        ///
        ///    FFmpeg_OnAVPacketMethodPtrT OnData;
        ///
        ///    OnData = &callbackFunction;
        ///
        ///    AVPacket *pkt;
        ///
        ///    ...
        ///
        ///    if (OnData != NULL)
        ///        OnData(pkt);
        /// \endcode
        ///
        /// Example of use with method:
        ///
        /// \code
        ///    class ExampleClass {
        ///    public:
        ///        void callbackFunction(AVPacket *data){
        ///            ...
        ///        }
        ///    };
        ///
        ///    ExampleClass obj;
        ///
        ///    FFmpeg_OnAVPacketMethodPtrT OnData;
        ///
        ///    OnData = FFmpeg_OnAVPacketMethodPtrT( &obj, &ExampleClass::callbackFunction );
        ///
        ///    AVPacket *pkt;
        ///
        ///    ...
        ///
        ///    if (OnData != NULL)
        ///        OnData(pkt);
        /// \endcode
        ///
        /// \author Alessandro Ribeiro
        ///
        DefineMethodPointer(FFmpeg_OnAVPacketMethodPtrT, void, AVPacket *data) VoidMethodCall(data)

        /// \class FFmpeg_OnAVFrameAVPacketMethodPtrT
        /// \brief Callback pattern with ffmpeg AVFrame and AVPacket parameters
        ///
        /// This callback was created to be used in the situation you have<br />
        /// A ffmpeg frame that was encoded into a ffmpeg packet.
        ///
        /// Example of use with functions:
        ///
        /// \code
        ///    void callbackFunction(AVFrame *input_frame, AVPacket *encoded_packet){
        ///        ...
        ///    }
        ///
        ///    FFmpeg_OnAVFrameAVPacketMethodPtrT OnData;
        ///
        ///    OnData = &callbackFunction;
        ///
        ///    AVFrame *frame;
        ///    AVPacket *packet;
        ///
        ///    ...
        ///
        ///    if (OnData != NULL)
        ///        OnData(frame, packet);
        /// \endcode
        ///
        /// Example of use with method:
        ///
        /// \code
        ///    class ExampleClass {
        ///    public:
        ///        void callbackFunction(AVFrame *input_frame, AVPacket *encoded_packet){
        ///            ...
        ///        }
        ///    };
        ///
        ///    ExampleClass obj;
        ///
        ///    FFmpeg_OnAVFrameAVPacketMethodPtrT OnData;
        ///
        ///    OnData = FFmpeg_OnAVFrameAVPacketMethodPtrT( &obj, &ExampleClass::callbackFunction );
        ///
        ///    AVFrame *frame;
        ///    AVPacket *packet;
        ///
        ///    ...
        ///
        ///    if (OnData != NULL)
        ///        OnData(frame, packet);
        /// \endcode
        ///
        /// \author Alessandro Ribeiro
        ///
        DefineMethodPointer(FFmpeg_OnAVFrameAVPacketMethodPtrT, void, AVFrame *input_frame, AVPacket *encoded_packet) VoidMethodCall(input_frame, encoded_packet)

        void globalInitFFMPEG();
}

#endif
