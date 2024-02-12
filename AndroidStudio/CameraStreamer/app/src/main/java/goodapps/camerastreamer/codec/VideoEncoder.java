package goodapps.camerastreamer.codec;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.os.Build;
import android.util.Log;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

public class VideoEncoder {

    public enum Type {
        None,
        HEVC,
        H264,
        _3GPP
    }

    public Type type;
    public String selected_mime_type;

    public Object opaqueData = null;

    public static final String TAG = "VideoEncoder";

    MediaCodecInfo codecInfo;
    int selectedFormat;
    boolean isPlanar;
    MediaCodec mEncoder;
    ByteBuffer[] writeBuffers;
    ByteBuffer[] readBuffers;
    int framerate;

    public List<byte[]> codecSpecificData = new ArrayList<byte[]>();

    int width;
    int height;
    int ySize;
    int uvSize;
    int uStart;
    int vStart;

    public Object _syncObj = new Object();

    Callback callback = null;

    long timestamp_from_start;
    long timestamp_holder;
    boolean first_time_query;

    ByteBuffer aux_buffer = null;

    public VideoEncoder(Type type, Object _opaqueData) {
        opaqueData = _opaqueData;
        this.type = type;
        switch(type){
            case HEVC:
                selected_mime_type = HardwareEncoderHelper.VIDEO_MIME_TYPE_HEVC;
                break;
            case H264:
                selected_mime_type = HardwareEncoderHelper.VIDEO_MIME_TYPE_H264;
                break;
            case _3GPP:
                selected_mime_type = HardwareEncoderHelper.VIDEO_MIME_TYPE_3GPP;
                break;
        }

        codecInfo = HardwareEncoderHelper.getVideoEncoder(selected_mime_type);
        selectedFormat = HardwareEncoderHelper.getFormat(codecInfo, selected_mime_type);
        isPlanar =
            (selectedFormat == MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Planar) ||
            (selectedFormat == MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420PackedPlanar)
            //(selectedFormat == MediaCodecInfo.CodecCapabilities.COLOR_FormatRGBFlexible) ||

        ;
    }

    public void setCallback(Callback callback) {
        this.callback = callback;
    }

    public boolean isCallbackSet(){
        return this.callback != null;
    }

    public void initialize(int width, int height, int bitrate, int framerate, int interval_between_keyframes_sec) {

        this.width = width;
        this.height = height;

        int w_aligned_16 = width;
        int h_aligned_16 = height;
        int h_uv_aligned_16 = h_aligned_16 / 2;

        int yStride = w_aligned_16;
        int uvStride = yStride / 2;
        ySize = yStride * h_aligned_16;
        uvSize = uvStride * h_uv_aligned_16;

        uStart = ySize;
        vStart = uStart + uvSize;

        this.framerate = framerate;

        //1mbps
        //bitrate = 1000000;
        //30fps
        //framerate = 30;
        //1 second
        //interval_between_keyframes_sec = 1;

        MediaFormat mediaFormat = MediaFormat.createVideoFormat(selected_mime_type, width, height);

        mediaFormat.setInteger(MediaFormat.KEY_BIT_RATE, bitrate);
        mediaFormat.setInteger(MediaFormat.KEY_FRAME_RATE, framerate);
        mediaFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT, selectedFormat);
        mediaFormat.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, interval_between_keyframes_sec);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            mediaFormat.setInteger(MediaFormat.KEY_LATENCY, 0);
        }

//        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
//            mediaFormat.setInteger(MediaFormat.KEY_LEVEL, MediaCodecInfo.CodecProfileLevel.HEVCProfileMain);
//        }

        /*
        mediaFormat.setInteger(MediaFormat.KEY_INTRA_REFRESH_PERIOD, 0);
        mediaFormat.setInteger(MediaFormat.KEY_LATENCY, 0);
        mediaFormat.setInteger(MediaFormat.KEY_MAX_HEIGHT, height);
        mediaFormat.setLong(MediaFormat.KEY_REPEAT_PREVIOUS_FRAME_AFTER, 1000000 / framerate);
         */


        try {
            mEncoder = MediaCodec.createByCodecName(codecInfo.getName());
            mEncoder.configure(mediaFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
            mEncoder.start();

            writeBuffers = mEncoder.getInputBuffers();
            readBuffers = mEncoder.getOutputBuffers();
        } catch (IOException e) {
            e.printStackTrace();
        }

        timestamp_from_start = 0;
        timestamp_holder = HardwareEncoderHelper.timestamp_micro();
        first_time_query = true;

        synchronized (codecSpecificData) {
            codecSpecificData.clear();
        }
    }

    public long computeTimeStamp_micro() {
        if (first_time_query) {
            first_time_query = false;
            timestamp_from_start = 0;
            timestamp_holder = HardwareEncoderHelper.timestamp_micro();
            return timestamp_from_start;
        }

        long now = HardwareEncoderHelper.timestamp_micro();
        timestamp_from_start += now - timestamp_holder;
        timestamp_holder = now;

        return timestamp_from_start;
    }

    public void encode420P(ByteBuffer frame){
        encode(frame);

        /*


        int size = width*height*3;

        if (aux_buffer == null || aux_buffer.capacity() != size){
            aux_buffer = ByteBuffer.allocateDirect(size);
        }

        frame.position(0);

        aux_buffer.clear();
        for(int h=0;h<height;h++){
            for(int x=0;x<width;x++) {
                byte y = frame.get();
                aux_buffer.put(y);
                aux_buffer.put(y);
                aux_buffer.put(y);
            }
        }


        encode(aux_buffer);

        */

    }

    public void encode(ByteBuffer frame) {

        // put data to the chip
        WriteBuffer writeBuffer = getWriteBuffer(1000000 / framerate);
        if (writeBuffer != null){
            if (writeBuffer.byteBuffer.capacity() < frame.position()){
                Log.e(TAG, "frame greater than the harware buffer input...");
                release();
                return;
            }

            writeBuffer.byteBuffer.clear();

            if (isPlanar)
                writeBuffer.byteBuffer.put(frame.array(), frame.arrayOffset(), frame.position());
            else {
                //need to interleave the U/V buffers
                byte[] inputData = frame.array();
                int offset = frame.arrayOffset();
                //int total_size_420 = frame.position();
                    /*
                ySize
                uvSize
                uStart
                vStart
                        */

                // write y buffer
                writeBuffer.byteBuffer.put(inputData, offset, ySize);

                // interleave u/v
                for(int i=0;i<uvSize;i++){
                    writeBuffer.byteBuffer.put(inputData[offset+uStart+i]);
                    writeBuffer.byteBuffer.put(inputData[offset+vStart+i]);
                }

            }


            postWriteBuffer(writeBuffer, 0, writeBuffer.byteBuffer.position(), computeTimeStamp_micro());//HardwareEncoderHelper.timestamp_micro()
        }


        // read NAL from the chip
        processAllOutput();

        /*
        ReadBuffer fromChip = getReadBuffer(1000000 / framerate);
        if (fromChip.result_action_or_buffer_index == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED){
            synchronized(_syncObj) {
                readBuffers = mEncoder.getOutputBuffers();
            }
        } else if (fromChip.byteBuffer != null) {
            byte[] encodedStream = new byte[fromChip.bufferInfo.size];
            fromChip.byteBuffer.clear();
            fromChip.byteBuffer.get(encodedStream, 0, fromChip.bufferInfo.size);
            releaseReadBuffer( fromChip );
        }
        */


    }


    public WriteBuffer getWriteBuffer(long timeoutUs){
        int bufferIndex = mEncoder.dequeueInputBuffer(timeoutUs);
        if (bufferIndex<0)
            return null;
        WriteBuffer result = new WriteBuffer();
        result.buffer_index = bufferIndex;
        result.byteBuffer = writeBuffers[bufferIndex];
        return result;
    }

    public void postWriteBuffer(WriteBuffer buffer, int offset, int size, long timestamp) {
        mEncoder.queueInputBuffer(buffer.buffer_index, offset, size, timestamp, 0);
    }

    public ReadBuffer getReadBuffer(long timeoutUs) {
        ReadBuffer result = new ReadBuffer();

        result.bufferInfo = new MediaCodec.BufferInfo();
        result.result_action_or_buffer_index = mEncoder.dequeueOutputBuffer(result.bufferInfo, timeoutUs);
        if (result.result_action_or_buffer_index >=0)
            result.byteBuffer = readBuffers[result.result_action_or_buffer_index];

        return result;
    }

    public void releaseReadBuffer(ReadBuffer buffer) {
        if (buffer.result_action_or_buffer_index < 0)
            return;
        mEncoder.releaseOutputBuffer(buffer.result_action_or_buffer_index, false);
    }

    public static class WriteBuffer {
        public int buffer_index;
        public ByteBuffer byteBuffer = null;
    }

    public static class ReadBuffer {
        public int result_action_or_buffer_index;
        public ByteBuffer byteBuffer = null;
        MediaCodec.BufferInfo bufferInfo;
    }


    public void flushMediaCodec() {
        ReadBuffer fromChip = getReadBuffer(1000000 / framerate);
        while (fromChip.result_action_or_buffer_index != MediaCodec.INFO_TRY_AGAIN_LATER) {
            if (fromChip.byteBuffer != null)
                releaseReadBuffer(fromChip);
            fromChip = getReadBuffer(1000000 / framerate);
        }
    }

    public void processAllOutput() {
        ReadBuffer fromChip = getReadBuffer(1000000 / framerate);
        while (fromChip.result_action_or_buffer_index != MediaCodec.INFO_TRY_AGAIN_LATER) {

            if (fromChip.result_action_or_buffer_index == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED){
                synchronized(_syncObj) {
                    readBuffers = mEncoder.getOutputBuffers();
                }
            } if (fromChip.result_action_or_buffer_index == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                MediaFormat newMediaFormat = mEncoder.getOutputFormat();

                // print all csd on output stream
                synchronized (codecSpecificData) {
                    codecSpecificData.clear();
                    MediaFormat format = mEncoder.getOutputFormat();
                    int csd_count = 0;
                    while (format.containsKey("csd-" + csd_count)) {
                        ByteBuffer binByteBuffer = format.getByteBuffer("csd-" + csd_count);
                        byte[] bin_byte = new byte[binByteBuffer.capacity()];
                        binByteBuffer.position(0);
                        binByteBuffer.get(bin_byte, 0, bin_byte.length);
                        codecSpecificData.add(bin_byte);

                        if (callback != null)
                            callback.onVideoEncoderData(bin_byte, opaqueData);

                        csd_count++;
                    }
                }

            } else if (fromChip.byteBuffer != null) {
                byte[] encodedStream = new byte[fromChip.bufferInfo.size];
                fromChip.byteBuffer.clear();
                fromChip.byteBuffer.get(encodedStream, 0, fromChip.bufferInfo.size);

                // On Stream Event...
                if (callback != null)
                    callback.onVideoEncoderData(encodedStream, opaqueData);

                releaseReadBuffer( fromChip );

                // H264: some devices does not send the output_format_changed... need to find the sps and pps by hand
                /*
                int csd_length = fromChip.bufferInfo.size;

                //check if the length fit into codecSpecificData buffer
                byte[] codecSpecificData = new byte[128];
                if (csd_length > h264_delimiter.length && csd_length <= codecSpecificData.length) {
                    fromChip.byteBuffer.clear();
                    fromChip.byteBuffer.get(codecSpecificData, 0, csd_length);

                    if (HardwareEncoderHelper.array_starts_with(codecSpecificData, h264_delimiter)) {

                        int next_delimiter = 4;
                        int index_start = 4;
                        while (next_delimiter<csd_length) {
                            // find the next h264_demiliter
                            next_delimiter = HardwareEncoderHelper.array_index_of(codecSpecificData, next_delimiter, csd_length, h264_delimiter);
                            int nal_type = codecSpecificData[index_start] & h264_NAL_TYPE_MASK;
                            if ( nal_type == h264_NAL_TYPE_SPS ) {
                                int size = next_delimiter-index_start;
                                synchronized(_syncObj) {
                                    sps = new byte[size];
                                    System.arraycopy(codecSpecificData, index_start, sps, 0, size);
                                }
                            } else if (  nal_type == h264_NAL_TYPE_PPS ) {
                                int size = next_delimiter-index_start;
                                synchronized(_syncObj) {
                                    pps = new byte[size];
                                    System.arraycopy(codecSpecificData, index_start, pps, 0, size);
                                }
                            }

                            //skip the h264_delimiter ( 0x 00 00 00 01 )
                            next_delimiter += 4;
                            index_start = next_delimiter;
                        }

                    }
                }

                releaseReadBuffer( fromChip );
                */
            }

            fromChip = getReadBuffer(1000000 / framerate);
        }
    }

    public void sendAllCSD() {
        for(byte[]csd:codecSpecificData){
            if (callback != null)
                callback.onVideoEncoderData(csd, opaqueData);
        }
    }

    public void release() {
        if (mEncoder != null) {
            flushMediaCodec();
            mEncoder.stop();
            mEncoder.release();
            mEncoder = null;
        }
        callback = null;
    }


    public interface Callback {
        void onVideoEncoderData(byte[] data, Object opaque);
    }
}
