#include "H264Utils.h"

#ifdef _WIN32
// win32 includes

#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <process.h>
#include <math.h>


enum PIPES { READ, WRITE }; /* Constants 0 and 1 for READ and WRITE */

#endif

namespace FFmpegWrapper {

    H264CheckNewFrame::H264CheckNewFrame() {
        log2_max_frame_num = 0x0;
        frame_num_mask = 0x0;
        last_frame_delta = 0x0;
        old_frame_num = 0;
        first_frame_num_set = false;
        newFrameOnNextIDR = false;
    }

    bool H264CheckNewFrame::isNewFrame(const uint8_t* input_nal, int input_size) {
        // check for 00 00 00 01 seq

        //bool validNAL = (input_size > 4) && array_index_of(input_nal, 0, input_size, h264_delimiter, 4) == 0;
        bool validNAL = (input_size > 4) && array_index_of(input_nal, 0, 4, h264_delimiter, 4) == 0;

        if (!validNAL) {
            printf("isThisNALANewFrame - Not valid NAL.\n");
            return false;
        }

        uint8_t nal_type = input_nal[4] & 0x1f;

        if (nal_type == h264_NAL_TYPE_SPS) {

            nal2RBSP(&input_nal[4], input_size - 4, &rbsp, 26);
            uint8_t* sps_raw = &rbsp[0];
            int sps_size = rbsp.size();

            // sps_raw[1]; // profile_idc
            // sps_raw[2]; // constraints
            // sps_raw[3]; // level_idc

            uint32_t bit_index = 8 + 24;//NAL bit + profile_idc (8bits) + constraints (8bits) + level_idc (8bits)
            bit_index += bitsToSkip_ue(bit_index, sps_raw, sps_size);//seq_parameter_set_id (ue)

            uint32_t log2_max_frame_num_minus4 = read_golomb_ue(bit_index, sps_raw, sps_size);

            ARIBEIRO_ABORT(
                (log2_max_frame_num_minus4 < 0 ||
                    log2_max_frame_num_minus4 > 12),
                "parseSPS_log2_max_frame_num_minus4 value not in range [0-12] \n");

            log2_max_frame_num = log2_max_frame_num_minus4 + 4;

            frame_num_mask = ((1 << log2_max_frame_num) - 1);
        }

        if (nal_type == (h264_NAL_TYPE_SPS) ||
            nal_type == (h264_NAL_TYPE_PPS) ||
            nal_type == (h264_NAL_TYPE_AUD) ||
            nal_type == (h264_NAL_TYPE_SEI) ||
            (nal_type >= 14 && nal_type <= 18)) {
            newFrameOnNextIDR = true;
        }
        else if (log2_max_frame_num != 0x0 &&
            (nal_type == h264_NAL_TYPE_CSIDRP || nal_type == h264_NAL_TYPE_CSNIDRP)) {

            rbsp.clear();
            //(8 + 3*(32*2+1) + 16) = max header per NALU slice bits = 27.375 bytes
            // 32 bytes + 8 (Possibility of Emulation in 32 bytes)
            int RBSPMaxBytes = 32 + 8;
            if ((input_size - 4) < (32 + 8))
                RBSPMaxBytes = input_size - 4;

            nal2RBSP(&input_nal[4], input_size - 4, &rbsp, RBSPMaxBytes);

            uint8_t * idr_raw = &rbsp[0];
            uint32_t idr_size = rbsp.size();

            uint32_t frame_num_index = 8;//start counting after the nal_bit
            frame_num_index += bitsToSkip_ue(frame_num_index, idr_raw, idr_size);//first_mb_in_slice (ue)
            frame_num_index += bitsToSkip_ue(frame_num_index, idr_raw, idr_size);//slice_type (ue)
            frame_num_index += bitsToSkip_ue(frame_num_index, idr_raw, idr_size);//pic_parameter_set_id (ue)

            //now can read frame_num
            uint32_t frame_num = readbits(frame_num_index, log2_max_frame_num, idr_raw, idr_size);

            if (!first_frame_num_set) {
                //first frame_num...
                first_frame_num_set = true;
                old_frame_num = frame_num;
            }

            if (old_frame_num != frame_num) {
                newFrameOnNextIDR = true;

                //frame_num will be in the range [0, ( 1 << spsinfo.log2_max_frame_num )[
                last_frame_delta = (frame_num - old_frame_num) & frame_num_mask;

                old_frame_num = frame_num;
            }
        }

        if (newFrameOnNextIDR &&
            (nal_type == h264_NAL_TYPE_CSIDRP || nal_type == h264_NAL_TYPE_CSNIDRP)) {
            newFrameOnNextIDR = false;

            printf(" ++ new frame\n");

            return true;
        }

        printf(" -- not new frame\n");
        return false;
    }

}