#ifndef YUV2RGB__HH__
#define YUV2RGB__HH__


ARIBEIRO_INLINE
static int iClamp(int v) {
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
#define YUV2GO(C, D, E) iClamp((C - 100 * (D) - 208 * (E) + 128) >> 8)
#define YUV2BO(C, D, E) iClamp((C + 516 * (D) + 128) >> 8)
// from: https://en.wikipedia.org/wiki/YUV - Y′UV444 to RGB888 conversion 
//
// RGB -> YUV conversion macros
//
#define RGB2Y(r, g, b) (((66 * (r) + 129 * (g) +  25 * (b) + 128) >> 8) +  16)
#define RGB2U(r, g, b) (((-38 * (r) - 74 * (g) + 112 * (b) + 128) >> 8) + 128)
#define RGB2V(r, g, b) (((112 * (r) - 94 * (g) -  18 * (b) + 128) >> 8) + 128)


struct YUV2RGB_Multithread_Job {
    int block;
    int blockStart;
    int blockEnd;
    int width;
    int height;
    const uint8_t* in_buffer;
    uint8_t* out_buffer;
};

class YUV2RGB_Multithread {
public:

    std::vector< aRibeiro::PlatformThread* > threads;
    int jobDivider;
    int threadCount;
    aRibeiro::PlatformSemaphore semaphore;
    aRibeiro::ObjectQueue<YUV2RGB_Multithread_Job> queue;

    void threadRun() {
        bool isSignaled;
        while (!aRibeiro::PlatformThread::isCurrentThreadInterrupted()) {
            YUV2RGB_Multithread_Job job = queue.dequeue(&isSignaled);
            if (isSignaled)
                break;

            //processing...
            for (int _y = job.blockStart; _y < job.blockEnd; _y++) {
            //for (int y = 0; y < job.height; y++) {
                for (int x = 0; x < job.width; x++) {
                    int index = x + _y * job.width;

                    int y = job.in_buffer[index * 2 + 0];

                    int u, v;
                    if (x % 2 == 0) {
                        u = job.in_buffer[index * 2 + 1];
                        v = job.in_buffer[(index + 1) * 2 + 1];
                    }
                    else {
                        u = job.in_buffer[(index - 1) * 2 + 1];
                        v = job.in_buffer[index * 2 + 1];
                    }

                    y = (y - 16) * 298;
                    u = u - 128;
                    v = v - 128;

                    //invert Y
                    index = x + (job.height - 1 - _y) * job.width;
                    
                    job.out_buffer[index * 4 + 0] = YUV2RO(y, u, v);
                    job.out_buffer[index * 4 + 1] = YUV2GO(y, u, v);
                    job.out_buffer[index * 4 + 2] = YUV2BO(y, u, v);
                }
            }

            semaphore.release();
        }
    }

public:


    void yuy2_to_rgba(const uint8_t* in_buffer, uint8_t* out_buffer, int width, int height) {

        YUV2RGB_Multithread_Job job;

        job.in_buffer = in_buffer;
        job.out_buffer = out_buffer;
        job.width = width;
        job.height = height;

        int division = jobDivider;
        int h_per_block = height / jobDivider;// +(((height % jobDivider) > 0) ? 1 : 0);
        
        if (height % jobDivider) {
            //or we choose divison + 1 or we choose h_per_block + 1
            int div_plus_1_total = (division + 1) * h_per_block;
            int h_per_block_plus_1_total = division * (h_per_block + 1);
            if (div_plus_1_total < h_per_block_plus_1_total)
                division++;
            else
                h_per_block++;
        }

        for (int block = 0; block < division; block++) {
            job.block = block;
            job.blockStart = block * h_per_block;
            job.blockEnd = (block + 1) * h_per_block;
            if (height < job.blockEnd)
                job.blockEnd = height;

            queue.enqueue(job);
        }

        for (int block = 0; block < division; block++)
            semaphore.blockingAcquire();
    }

    YUV2RGB_Multithread(int threadCount, int jobDivider) :semaphore(0) {
        this->jobDivider = jobDivider;
        this->threadCount = threadCount;
        for (int i = 0; i < threadCount; i++) {
            threads.push_back(new aRibeiro::PlatformThread(this, &YUV2RGB_Multithread::threadRun));
        }
        for (int i = 0; i < threads.size(); i++)
            threads[i]->start();
    }

    void finalizeThreads() {
        printf("[YUV2RGB_Multithread] finalize threads start\n");

        for (int i = 0; i < threads.size(); i++)
            threads[i]->interrupt();
        for (int i = 0; i < threads.size(); i++)
            threads[i]->wait();

        printf("[YUV2RGB_Multithread] finalize threads done\n");
    }

    virtual ~YUV2RGB_Multithread() {
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

#endif