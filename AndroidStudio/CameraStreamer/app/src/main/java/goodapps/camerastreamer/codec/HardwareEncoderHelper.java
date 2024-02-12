package goodapps.camerastreamer.codec;

import android.media.MediaCodecInfo;
import android.media.MediaCodecList;

import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.LinkedList;
import java.util.List;

public class HardwareEncoderHelper {


    public static final String VIDEO_MIME_TYPE_H264 = "video/avc";
    public static final String VIDEO_MIME_TYPE_HEVC = "video/hevc";
    public static final String VIDEO_MIME_TYPE_3GPP = "video/3gpp";
    public static final String VIDEO_MIME_TYPE_VP9 = "video/x-vnd.on2.vp9";

    public static final Integer[] FORMATS_IMPLEMENTED = {
            //planar types (the format the camera preview already returns)
            MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Planar,
            MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420PackedPlanar,

            //non-planar types (like the NV21 - but with V and U as input -- need to interleave the UV buffer)
            MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar,
            MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420PackedSemiPlanar,
            MediaCodecInfo.CodecCapabilities.COLOR_TI_FormatYUV420PackedSemiPlanar
    };

    public static final List<Integer> ALL_FORMATS_LIST = Arrays.asList(FORMATS_IMPLEMENTED);

    public static boolean checkIsCompatible(MediaCodecInfo codec, String VIDEO_MIME_TYPE) {
        MediaCodecInfo.CodecCapabilities capabilities = codec.getCapabilitiesForType(VIDEO_MIME_TYPE);

        for(int formatID:capabilities.colorFormats){
            if (ALL_FORMATS_LIST.contains(formatID))
                return true;
        }

        return false;
    }

    public static List<MediaCodecInfo> getCodecEncodersAll() {
        List<MediaCodecInfo> codecs = new LinkedList<>();

        // start from the latest codec (trying support 420 planar first)
        //for(int i = MediaCodecList.getCodecCount()-1; i >= 0 ; i--) {
        for(int i = 0; i < MediaCodecList.getCodecCount() ; i++) {
            MediaCodecInfo codecInfo = MediaCodecList.getCodecInfoAt(i);
            if (codecInfo.isEncoder()) {
                codecs.add(codecInfo);
            }
        }

        return codecs;
    }


    public static MediaCodecInfo getVideoEncoder(String VIDEO_MIME_TYPE) {
        for(MediaCodecInfo codec:getCodecEncodersAll()){
            for(String supportedFormat:codec.getSupportedTypes()){
                if (supportedFormat.equalsIgnoreCase(VIDEO_MIME_TYPE)){
                    if (checkIsCompatible(codec, VIDEO_MIME_TYPE))
                        return codec;
                }
            }
        }
        return null;
    }

    public static int getFormat( MediaCodecInfo videoCodec, String VIDEO_MIME_TYPE) {
        MediaCodecInfo.CodecCapabilities capabilities = videoCodec.getCapabilitiesForType(VIDEO_MIME_TYPE);
        for(int formatID:capabilities.colorFormats){
            if (ALL_FORMATS_LIST.contains(formatID))
                return formatID;
        }
        return -1;
    }


    public static long timestamp_micro() {
        return System.nanoTime()/1000;
    }

    public static boolean array_equals(byte[] a, int offset_a, byte[] b, int offset_b, int len_to_compare) {
        if (offset_a + len_to_compare < a.length || offset_b + len_to_compare < b.length)
            return false;
        return ByteBuffer.wrap(a, offset_a, len_to_compare).equals(ByteBuffer.wrap(b, offset_b, len_to_compare));
    }

    public static boolean array_starts_with(byte[] a, byte[] b) {
        if (a.length < b.length)
            return false;
        return array_equals(a, 0, b, 0, b.length);
    }

    // returns the input.length, if dont find the pattern
    public static int array_index_of(byte[]input, int start_input,int input_size, byte[] pattern) {

        int test_limit = input_size - pattern.length;

        for(int i=start_input;i<=test_limit;i++){
            boolean equals = true;
            for( int j=0;j<pattern.length;j++){
                if (input[i+j] != pattern[j]){
                    equals = false;
                    break;
                }
            }
            if (equals)
                return i;
        }

        return input_size;
    }

}
