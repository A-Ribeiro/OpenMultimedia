#pragma once

#include <InteractiveToolkit/EventCore/Callback.h>
#include <InteractiveToolkit/ITKCommon/ITKAbort.h>

namespace FFmpegWrapper {

    const uint8_t h264_delimiter[] = { 0x00, 0x00, 0x00, 0x01 };
    const uint8_t h264_NAL_TYPE_SPS = 0x07; //   Sequence parameter set
    const uint8_t h264_NAL_TYPE_PPS = 0x08; //   Picture parameter set
    const uint8_t h264_NAL_TYPE_AUD = 0x09; //   Access unit delimiter
    const uint8_t h264_NAL_TYPE_SEI = 0x06; //   Supplemental enhancement information (SEI)
    const uint8_t h264_NAL_TYPE_CSIDRP = 0x05; //   Coded slice of an IDR picture
    const uint8_t h264_NAL_TYPE_CSNIDRP = 0x01; // Coded slice of a non-IDR picture

    ITK_INLINE
    static int array_index_of(const uint8_t *input, int start_input, int input_size, const uint8_t *pattern, int pattern_size) {
        int test_limit = input_size - pattern_size;
        for (int i = start_input; i <= test_limit; i++) {
            if (memcmp(&input[i], pattern, pattern_size) == 0)
                return i;
        }
        return input_size;
    }

    enum State {
        None,
        Data,
        NAL0,
        NAL1,

        EMULATION0,
        EMULATION1,
        EMULATION_FORCE_SKIP
    };

    // Raw Byte Sequence Payload (RBSP) -- without the emulation prevention bytes
    // maxSize is used to parse just the beggining of the nal structure...
    // avoid process all buffer input_size on NAL new frame check
    ITK_INLINE
    static void nal2RBSP(const uint8_t *buffer, size_t size,
        std::vector<uint8_t> *rbsp,
        int maxSize) {
        if (maxSize <= 0 || (size < maxSize))
            maxSize = size;
        rbsp->resize(maxSize);

        State emulationState = None;
        int count = 0;

        for (int i = 0; i < maxSize; i++) {
            uint8_t byte = buffer[i];

            if (byte == 0x00 && emulationState == None)
                emulationState = EMULATION0;
            else if (byte == 0x00 && (emulationState == EMULATION0 || emulationState == EMULATION1))
                emulationState = EMULATION1;
            else if (byte == 0x03 && emulationState == EMULATION1)
            {
                emulationState = EMULATION_FORCE_SKIP;
                continue;
            }
            else if (emulationState == EMULATION_FORCE_SKIP) { //skip 00 01 02 or 03

                ITK_ABORT(
                    (byte != 0x00 && byte != 0x01 && byte != 0x02 && byte != 0x03),
                    "H264 NAL emulation prevention pattern detected error (%u)\n", byte);

                //if ( byte != 0x00 && byte != 0x01 && byte != 0x02 && byte != 0x03 ){
                //    fprintf(stdout, "H264 NAL emulation prevention pattern detected error (%u)\n", byte);
                //    exit(-1);
                //}
                emulationState = None;
            }
            else
                emulationState = None;

            (*rbsp)[count] = byte;
            count++;
        }
        if (count != rbsp->size())
            rbsp->resize(count);
    }

    ITK_INLINE
    static uint32_t readbit(int bitPos, const uint8_t* data, int size) {
        //int dataPosition = bitPos / 8;
        int dataPosition = bitPos >> 3;
        int bitPosition = 7 - bitPos % 8;

        ITK_ABORT(
            (dataPosition >= size),
            "error to access bit...\n");

        //if (dataPosition >= size){
        //    fprintf(stderr,"error to access bit...\n");
        //    exit(-1);
        //}

        return (data[dataPosition] >> bitPosition) & 0x01;
    }

    ITK_INLINE
    static uint32_t readbits(int bitPos, int length, const uint8_t* data, int size) {
        ITK_ABORT(
            (length > 32),
            "readbits length greated than 32\n");

        //if (length > 32){
        //    fprintf(stderr,"readbits length greated than 32\n");
        //    exit(-1);
        //}

        uint32_t result = 0;
        for (int i = 0; i < length; i++) {
            result <<= 1;
            result = result | readbit(bitPos + i, data, size);
        }
        return result;
    }

    // leading 0`s count equals the number of next bits after bit 1
    //
    //  Example: 01x  001xx 0001xxx 00001xxxx
    //
    //  The max number of bits is equal 32 in this sintax
    //
    static int bitsToSkip_ue(int bit_start, const uint8_t* data, int size) {
        int bitPos = bit_start;
        //int dataPosition = bit_start / 8;
        int dataPosition = bit_start >> 3;
        int bitPosition;
        while (dataPosition < size) {
            //dataPosition = bitPos / 8;
            dataPosition = bitPos >> 3;
            bitPosition = 7 - bitPos % 8;
            int bit = (data[dataPosition] >> bitPosition) & 0x01;
            if (bit == 1)
                break;
            bitPos++;
        }
        int leadingZeros = bitPos - bit_start;
        int totalBits = leadingZeros + 1 + leadingZeros;

        ITK_ABORT(
            (totalBits > 32),
            "bitsToSkip_ue length greated than 32\n");

        //if (totalBits > 32){
        //    fprintf(stderr,"bitsToSkip_ue length greated than 32\n");
        //    exit(-1);
        //}

        return totalBits;
    }

    ITK_INLINE
    static uint32_t read_golomb_ue(int bit_start, const uint8_t* data, int size) {
        int bitPos = bit_start;
        //int dataPosition = bit_start / 8;
        int dataPosition = bit_start >> 3;
        int bitPosition;
        while (dataPosition < size) {
            //dataPosition = bitPos / 8;
            dataPosition = bitPos >> 3;
            bitPosition = 7 - bitPos % 8;
            int bit = (data[dataPosition] >> bitPosition) & 0x01;
            if (bit == 1)
                break;
            bitPos++;
        }
        uint32_t leadingZeros = (uint32_t)(bitPos - bit_start);
        uint32_t num = readbits(bitPos + 1, leadingZeros, data, size);
        num += (1 << leadingZeros) - 1;

        return num;
    }


    // Network Abstraction Layer (NAL)
    // instantaneous decoding refresh (IDR)
    class H264CheckNewFrame {

    public:

        // Raw Byte Sequence Payload (RBSP)
        std::vector<uint8_t> rbsp;

        uint32_t log2_max_frame_num; // from SPS information
        uint32_t frame_num_mask;
        uint32_t last_frame_delta;
        
        uint32_t old_frame_num;
        bool first_frame_num_set;

        bool newFrameOnNextIDR;

        H264CheckNewFrame();

        bool isNewFrame(const uint8_t* input_nal, int input_size);
    };
    
}
