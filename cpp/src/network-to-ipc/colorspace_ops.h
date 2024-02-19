#pragma once

#include <InteractiveToolkit/ITKCommon/Memory.h>
#include <InteractiveToolkit/Platform/Thread.h>
#include <InteractiveToolkit/Platform/Semaphore.h>
#include <InteractiveToolkit/Platform/Core/ObjectQueue.h>

#include <stdint.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#if defined(ITK_SSE2)
#ifdef _MSC_VER //_WIN32
#include <intrin.h>
#else
#include <x86intrin.h> // Everything on SIMD...
#endif
#elif defined(ITK_NEON)
#include <arm_neon.h>
#endif

ITK_INLINE
static int iClamp(int v)
{
    if (v > 255)
        return 255;
    else if (v < 0)
        return 0;
    else
        return v;
}
// from: https://en.wikipedia.org/wiki/YUV - Y′UV444 to RGB888 conversion
//
// C = (Y - 16)*298
// D = U - 128
// E = V - 128
//
#define YUV2RO(C, D, E) iClamp((C + 409 * (E) + 128) >> 8)
#define YUV2GO(C, D, E) iClamp((C - 100 * (D)-208 * (E) + 128) >> 8)
#define YUV2BO(C, D, E) iClamp((C + 516 * (D) + 128) >> 8)
// from: https://en.wikipedia.org/wiki/YUV - Y′UV444 to RGB888 conversion
//
// RGB -> YUV conversion macros
//
#define RGB2Y(r, g, b) (((66 * (r) + 129 * (g) + 25 * (b) + 128) >> 8) + 16)
#define RGB2U(r, g, b) (((-38 * (r)-74 * (g) + 112 * (b) + 128) >> 8) + 128)
#define RGB2V(r, g, b) (((112 * (r)-94 * (g)-18 * (b) + 128) >> 8) + 128)

const uint32_t FILTER_PRECISION_BITS = 10; // 1024
const uint32_t FILTER_TOTAL_COUNT = (1 << FILTER_PRECISION_BITS);
const uint32_t FILTER_MASK = FILTER_TOTAL_COUNT - 1;
const uint32_t FILTER_HALF = (FILTER_MASK >> 1);

#define FILTER_NEAREST 1
#define FILTER_LINEAR 2

#define FILTER_SELECT FILTER_LINEAR

// Y′UV422 (y1, u, y2, v) -> https://en.wikipedia.org/wiki/YUV
static uint8_t *alloc_yuy2_aligned(int width, int height)
{
    int size = width * height * 2;
    uint8_t *result = (uint8_t *)ITKCommon::Memory::malloc(size);
    memset(result, 127, size);
    return result;
}

static uint8_t *alloc_rgb_to_rgbx_aligned(uint8_t *rgb, int w, int h)
{
    int size = w * h * 4;
    uint8_t *result = (uint8_t *)ITKCommon::Memory::malloc(size);
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            int input_index = (y * w + x) * 3;
            int output_index = (y * w + x) * 4;
            result[output_index + 0] = rgb[input_index + 0];
            result[output_index + 1] = rgb[input_index + 1];
            result[output_index + 2] = rgb[input_index + 2];
        }
    }
    return result;
}

static void rgbx_to_yuy2(uint8_t *rgba, uint8_t *yuy2, int w, int h)
{

    uint32_t *rgba32 = (uint32_t *)rgba;
    uint32_t *yuy2_32 = (uint32_t *)yuy2;

    /*

    for (int y = 0; y < h; y++) {
        int rgba_line = y * w;
        int yuyv_line = (y * w) >> 1;
        for (int x = 0; x < w; x += 2) {

            int rgb_access = rgba_line + x;
            uint32_t rgb_a = rgba32[rgb_access];

            uint8_t r_a = rgb_a & 0xff;
            uint8_t g_a = (rgb_a >> 8) & 0xff;
            uint8_t b_a = (rgb_a >> 16) & 0xff;

            uint32_t rgb_b = rgba32[rgb_access + 1];

            uint8_t r_b = rgb_b & 0xff;
            uint8_t g_b = (rgb_b >> 8) & 0xff;
            uint8_t b_b = (rgb_b >> 16) & 0xff;


            uint32_t yuyv;

            yuyv = (RGB2Y(r_a, g_a, b_a) & 0xff);
            yuyv |= (RGB2U(r_a, g_a, b_a) & 0xff) << 8;
            yuyv |= (RGB2Y(r_b, g_b, b_b) & 0xff) << 16;
            yuyv |= (RGB2V(r_a, g_a, b_a) & 0xff) << 24;

            int yuy2_access = yuyv_line + (x >> 1);
            yuy2_32[yuy2_access] = yuyv;
        }
    }

    */

    // #pragma omp parallel for
    for (int _x = 0; _x < w * h; _x += 2)
    {

        int x = _x % w;
        int y = _x / w;

        int rgba_line = y * w;
        int yuyv_line = (y * w) >> 1;

        int rgb_access = rgba_line + x;
        uint32_t rgb_a = rgba32[rgb_access];

        uint8_t r_a = rgb_a & 0xff;
        uint8_t g_a = (rgb_a >> 8) & 0xff;
        uint8_t b_a = (rgb_a >> 16) & 0xff;

        uint32_t rgb_b = rgba32[rgb_access + 1];

        uint8_t r_b = rgb_b & 0xff;
        uint8_t g_b = (rgb_b >> 8) & 0xff;
        uint8_t b_b = (rgb_b >> 16) & 0xff;

        uint32_t yuyv;

        yuyv = (RGB2Y(r_a, g_a, b_a) & 0xff);
        yuyv |= (RGB2U(r_a, g_a, b_a) & 0xff) << 8;
        yuyv |= (RGB2Y(r_b, g_b, b_b) & 0xff) << 16;
        yuyv |= (RGB2V(r_a, g_a, b_a) & 0xff) << 24;

        int yuy2_access = yuyv_line + (x >> 1);
        yuy2_32[yuy2_access] = yuyv;
    }
}

ITK_INLINE
static int filter_int_lerp(int a, int b, int lrp)
{
    int one_minus_lrp = FILTER_MASK - lrp;
    int result = (a * one_minus_lrp + b * lrp) >> FILTER_PRECISION_BITS;
    return result;
}

///  D-f(0,1) ---*----- C-f(1,1)
///     |        |         |
///     |        |         |
/// .   *--------P---------*   P = (dx,dy)
///     |        |         |
///     |        |         |
///  A-f(0,0) ---*----- B-f(1,0)
ITK_INLINE
static int filter_int_blerp(int A, int B, int C, int D,
                            int dx, int dy)
{

    int one_minus_dx = FILTER_MASK - dx;
    int one_minus_dy = FILTER_MASK - dy;

    /*
    // x pass
    int a_b = (A * one_minus_dx + B * dx) >> FILTER_PRECISION_BITS;
    int d_c = (D * one_minus_dx + C * dx) >> FILTER_PRECISION_BITS;

    // y pass
    return (a_b * one_minus_dy + d_c * dy) >> FILTER_PRECISION_BITS;
    */

    int a = one_minus_dx * one_minus_dy * A;
    int b = dx * one_minus_dy * B;
    int c = dx * dy * C;
    int d = one_minus_dx * dy * D;

    int result_a = (a + c) >> FILTER_PRECISION_BITS;
    int result_b = (b + d) >> FILTER_PRECISION_BITS;

    int result = (result_a + result_b) >> FILTER_PRECISION_BITS;

    return result;
}

ITK_INLINE
static int8_t filter_pixel(
    const uint8_t *yuv420_buffer,

    int _420_x,
    int _420_y,

    int _420_stride,

    int _dst_x,
    int _dst_y,

    int _numerator,
    int _denominator_shift)
{

#if FILTER_SELECT == FILTER_NEAREST

    int index_420 = (_420_y * _420_stride + _420_x);
    int8_t y = yuv420_buffer[index_420];

#elif FILTER_SELECT == FILTER_LINEAR
    // the formula to convert the dst to 420 is:
    //
    //  _420_x = ( _dst_x * _numerator ) >> _denominator_shift;
    //
    int shift_diff = _denominator_shift - FILTER_PRECISION_BITS;
    uint32_t _x_hp = _dst_x * _numerator;
    uint32_t _y_hp = _dst_y * _numerator;

    if (shift_diff < 0)
    {
        shift_diff = -shift_diff;
        _x_hp = _x_hp << shift_diff;
        _y_hp = _y_hp << shift_diff;
    }
    else
    {
        _x_hp = _x_hp >> shift_diff;
        _y_hp = _y_hp >> shift_diff;
    }

    // do x_lerp
    uint32_t _x_hp_fraction = _x_hp & FILTER_MASK;
    uint32_t _y_hp_fraction = _y_hp & FILTER_MASK;
    // int _x_hp_access = (_x_hp >> FILTER_PRECISION_BITS) + center_yuv_420_x;

    int _x_min, _x_max;
    int _y_min, _y_max;

    if (_x_hp_fraction > FILTER_HALF)
    {
        // |    *  f | -> lrp = f-*
        // use the next
        _x_min = _420_x;
        _x_max = _420_x + 1;

        _x_hp_fraction -= FILTER_HALF;
    }
    else
    {
        // |  f *    | -> lrp = f+*
        // use the previous
        _x_min = _420_x - 1;
        _x_max = _420_x;

        _x_hp_fraction += FILTER_HALF;
    }

    if (_y_hp_fraction > FILTER_HALF)
    {
        // |    *  f | -> lrp = f-*
        // use the next
        _y_min = _420_y;
        _y_max = _420_y + 1;

        _y_hp_fraction -= FILTER_HALF;
    }
    else
    {
        // |  f *    | -> lrp = f+*
        // use the previous
        _y_min = _420_y - 1;
        _y_max = _420_y;

        _y_hp_fraction += FILTER_HALF;
    }

    // test X
    /*
    int index_x_420_a = (_420_y * _420_stride + _x_min);
    int index_x_420_b = (_420_y * _420_stride + _x_max);
    int8_t y = filter_int_lerp(yuv420_buffer[index_x_420_a], yuv420_buffer[index_x_420_b], _x_hp_fraction);
    // */

    // test y
    /*
    int index_y_420_a = (_y_min * _420_stride + _420_x);
    int index_y_420_b = (_y_max * _420_stride + _420_x);
    int8_t y = filter_int_lerp(yuv420_buffer[index_y_420_a], yuv420_buffer[index_y_420_b], _y_hp_fraction);
    // */

    // blerp
    ///  D-f(0,1) ---*----- C-f(1,1)
    ///     |        |         |
    ///     |        |         |
    /// .   *--------P---------*   P = (dx,dy)
    ///     |        |         |
    ///     |        |         |
    ///  A-f(0,0) ---*----- B-f(1,0)
    int index_a = (_x_min + _y_min * _420_stride); // (0,0)
    int index_b = (_x_max + _y_min * _420_stride); // (1,0)
    int index_c = (_x_max + _y_max * _420_stride); // (1,1)
    int index_d = (_x_min + _y_max * _420_stride); // (0,1)

    int8_t y = filter_int_blerp(
        yuv420_buffer[index_a], yuv420_buffer[index_b], yuv420_buffer[index_c], yuv420_buffer[index_d],
        _x_hp_fraction, _y_hp_fraction);

    // */
#endif

    return y;
}

// find denominator power 2 shift
ITK_INLINE
static void computeNumeratorDenominatorShift(int in_num, int in_deno, int *out_num, int *out_shift)
{
    // find full_deno power 2 shift
    int deno_shift = 0;
    int new_deno_shift_aux = 1 << deno_shift;

    while (new_deno_shift_aux < in_deno)
    {
        new_deno_shift_aux <<= 1;
        deno_shift++;
    }

    *out_shift = deno_shift;

    // recompute the _numerator and full_deno
    *out_num = (in_num * new_deno_shift_aux) / in_deno;
}

struct CopyRescaleMultithread_Job
{

    int block;
    int blockStart;
    int blockEnd;

    int yuy_w;
    int yuy_stride;
    int half_num;
    int half_deno_shift;
    int half_center_yuv_420_x;
    int half_center_yuv_420_y;

    int yuv420_w;
    int yuv420_h;

    int full_num;
    int full_deno_shift;

    int center_yuv_420_x;
    int center_yuv_420_y;

    const uint8_t *yuv420;
    uint8_t *yuy2;

    int yuv_420_stride_y;
    int yuv_420_stride_uv;

    int src_v_start_index;
    int src_u_start_index;
};

class CopyRescaleMultithread : public EventCore::HandleCallback
{
    std::vector<Platform::Thread *> threads;
    int jobDivider;
    int threadCount;
    Platform::Semaphore semaphore;
    Platform::ObjectQueue<CopyRescaleMultithread_Job> queue;

    void threadRun()
    {
        bool isSignaled;
        while (!Platform::Thread::isCurrentThreadInterrupted())
        {
            CopyRescaleMultithread_Job job = queue.dequeue(&isSignaled);
            if (isSignaled)
                break;

            for (int _y = job.blockStart; _y < job.blockEnd; _y++)
            {

                for (int _x = 0; _x < job.yuy_w; _x++)
                {

                    int index_y = (_y * job.yuy_stride + _x) << 1;
                    int index_u_or_v = index_y + 1;

                    int _x_dst_pos = _x;
                    int _y_dst_pos = _y;

                    int half_420_x = ((_x_dst_pos * job.half_num) >> job.half_deno_shift) + job.half_center_yuv_420_x;
                    int half_420_y = ((_y_dst_pos * job.half_num) >> job.half_deno_shift) + job.half_center_yuv_420_y;

                    if (half_420_x >= 2 && half_420_x < ((job.yuv420_w >> 1) - 2) &&
                        half_420_y >= 0 && half_420_y < (job.yuv420_h >> 1))
                    {

                        int _420_x = ((_x_dst_pos * job.full_num) >> job.full_deno_shift) + job.center_yuv_420_x;
                        int _420_y = ((_y_dst_pos * job.full_num) >> job.full_deno_shift) + job.center_yuv_420_y;

                        job.yuy2[index_y] = filter_pixel(
                            job.yuv420,
                            _420_x, _420_y,
                            job.yuv_420_stride_y,
                            _x_dst_pos, _y_dst_pos,
                            job.full_num, job.full_deno_shift);

                        int uv_start_index;
                        if (_x % 2 != 0)
                        {

                            // process (UV) data
                            // the V value is related to the previous X position of the destination buffer
                            _x_dst_pos -= 1; // before pixel processing for 'v' from 'uv' buffer
                            half_420_x = ((_x_dst_pos * job.half_num) >> job.half_deno_shift) + job.half_center_yuv_420_x;

                            uv_start_index = job.src_v_start_index;
                        }
                        else
                            uv_start_index = job.src_u_start_index;

                        job.yuy2[index_u_or_v] = filter_pixel(
                            &job.yuv420[uv_start_index],
                            half_420_x, half_420_y,
                            job.yuv_420_stride_uv,
                            _x_dst_pos, _y_dst_pos,
                            job.half_num, job.half_deno_shift);
                    }
                    else
                    {
                        // black
                        job.yuy2[index_y] = 16;
                        job.yuy2[index_u_or_v] = 128;
                    }
                }
            }

            semaphore.release();
        }
    }

public:
    void yuv420_to_yuy2_copy_rescale(const uint8_t *yuv420, int yuv420_w, int yuv420_h, uint8_t *yuy2, int yuy_w, int yuy_h)
    {

        int center_yuy_x = yuy_w / 2;
        int center_yuy_y = yuy_h / 2;

        float aspect_yuv = (float)yuv420_w / (float)yuv420_h;
        float aspect_yuy = (float)yuy_w / (float)yuy_w;
        bool fit_width = (aspect_yuv <= aspect_yuy);

        int full_num = 1;
        int full_deno_shift = 0;

        int half_num = 1;
        int half_deno_shift = 0;

        int px_to_remove = 8;

        if (!fit_width)
        {
            // fit width
            computeNumeratorDenominatorShift(yuv420_w - px_to_remove, yuy_w, &full_num, &full_deno_shift);
            computeNumeratorDenominatorShift((yuv420_w - px_to_remove) >> 1, yuy_w, &half_num, &half_deno_shift);
        }
        else
        {
            // fit height
            computeNumeratorDenominatorShift(yuv420_h - px_to_remove, yuy_h, &full_num, &full_deno_shift);
            computeNumeratorDenominatorShift((yuv420_h - px_to_remove) >> 1, yuy_h, &half_num, &half_deno_shift);
        }

        px_to_remove = 0;

        int center_yuv_420_x = ((yuv420_w - px_to_remove) >> 1) - ((center_yuy_x * full_num) >> full_deno_shift) - 2;
        int center_yuv_420_y = ((yuv420_h - px_to_remove) >> 1) - ((center_yuy_y * full_num) >> full_deno_shift) - 2;

        int half_center_yuv_420_x = ((yuv420_w - px_to_remove) >> 2) - ((center_yuy_x * half_num) >> half_deno_shift) - 1;
        int half_center_yuv_420_y = ((yuv420_h - px_to_remove) >> 2) - ((center_yuy_y * half_num) >> half_deno_shift) - 1;

        int yuv_420_stride_y = yuv420_w;
        int yuv_420_stride_uv = yuv420_w >> 1;

        int yuv_420_y_total_size = yuv_420_stride_y * yuv420_h;
        int yuv_420_uv_total_size = yuv_420_stride_uv * (yuv420_h >> 1);

        int src_u_start_index = yuv_420_y_total_size;
        int src_v_start_index = src_u_start_index + yuv_420_uv_total_size;

        int yuy_stride = yuy_w;

        // int h_per_block = yuy_h / jobDivider + (((yuy_h % jobDivider) > 0) ? 1 : 0);

        int division = jobDivider;
        int h_per_block = yuy_h / jobDivider; // +(((height % jobDivider) > 0) ? 1 : 0);

        if (yuy_h % jobDivider)
        {
            // or we choose divison + 1 or we choose h_per_block + 1
            int div_plus_1_total = (division + 1) * h_per_block;
            int h_per_block_plus_1_total = division * (h_per_block + 1);
            if (div_plus_1_total < h_per_block_plus_1_total)
                division++;
            else
                h_per_block++;
        }

        CopyRescaleMultithread_Job job;

        job.yuy_w = yuy_w;
        job.yuy_stride = yuy_stride;
        job.half_num = half_num;
        job.half_deno_shift = half_deno_shift;
        job.half_center_yuv_420_x = half_center_yuv_420_x;
        job.half_center_yuv_420_y = half_center_yuv_420_y;

        job.yuv420_w = yuv420_w;
        job.yuv420_h = yuv420_h;

        job.full_num = full_num;
        job.full_deno_shift = full_deno_shift;

        job.center_yuv_420_x = center_yuv_420_x;
        job.center_yuv_420_y = center_yuv_420_y;

        job.yuv420 = yuv420;
        job.yuy2 = yuy2;

        job.yuv_420_stride_y = yuv_420_stride_y;
        job.yuv_420_stride_uv = yuv_420_stride_uv;

        job.src_v_start_index = src_v_start_index;
        job.src_u_start_index = src_u_start_index;


        int jobs_dispatched = 0;
        int block = 0;

        job.block = block;
        job.blockStart = block * h_per_block;
        job.blockEnd = (block + 1) * h_per_block;

        while ( job.blockStart < yuy_h )
        {
            if (yuy_h < job.blockEnd)
                job.blockEnd = yuy_h;

            queue.enqueue(job);

            jobs_dispatched ++;
            block ++;
            job.block = block;
            job.blockStart = block * h_per_block;
            job.blockEnd = (block + 1) * h_per_block;
        }

        for (int block = 0; block < jobs_dispatched; block++)
            semaphore.blockingAcquire();
    }

    CopyRescaleMultithread(int threadCount, int jobDivider) : semaphore(0)
    {
        this->jobDivider = jobDivider;
        this->threadCount = threadCount;
        for (int i = 0; i < threadCount; i++)
        {
            threads.push_back(new Platform::Thread(EventCore::CallbackWrapper(&CopyRescaleMultithread::threadRun, this)));
        }
        for (int i = 0; i < threads.size(); i++)
            threads[i]->start();
    }

    void finalizeThreads()
    {
        printf("[CopyRescaleMultithread] finalize threads start\n");

        for (int i = 0; i < threads.size(); i++)
            threads[i]->interrupt();
        for (int i = 0; i < threads.size(); i++)
            threads[i]->wait();

        printf("[CopyRescaleMultithread] finalize threads done\n");
    }

    virtual ~CopyRescaleMultithread()
    {
        for (int i = 0; i < threads.size(); i++)
            threads[i]->interrupt();
        for (int i = 0; i < threads.size(); i++)
            threads[i]->wait();
        for (int i = 0; i < threads.size(); i++)
            delete threads[i];
        threads.clear();
        threadCount = 0;
    }
};
