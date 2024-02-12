#include <InteractiveToolkit/InteractiveToolkit.h>
#include <InteractiveToolkit-Extension/InteractiveToolkit-Extension.h>

#import <Foundation/Foundation.h>
#include <Syphon/Syphon.h>
//#include <OpenGL/gl3.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLTypes.h>
#include <OpenGL/CGLCurrent.h>
#include <OpenGL/gl3.h>

#include "GLTexture.h"

#include "YUV2RGB.h"

class GLContextWithoutWindow {
public:
    CGLContextObj context;

    GLContextWithoutWindow() {
        CGLPixelFormatAttribute attributes[] = {
            kCGLPFAAccelerated,   // no software rendering
            kCGLPFAOpenGLProfile, // core profile with the version stated below
            (CGLPixelFormatAttribute) kCGLOGLPVersion_3_2_Core,
            //(CGLPixelFormatAttribute) kCGLOGLPVersion_Legacy,
            (CGLPixelFormatAttribute) 0
        };

        CGLPixelFormatObj pix;
        CGLError errorCode;
        GLint num; // stores the number of possible pixel formats
        errorCode = CGLChoosePixelFormat( attributes, &pix, &num );
        // add error checking here
        errorCode = CGLCreateContext( pix, NULL, &context ); // second parameter can be another context for object sharing
        // add error checking here
        ITK_ABORT(errorCode, "Error CGLCreateContext\n");

        CGLDestroyPixelFormat( pix );
        
        errorCode = CGLSetCurrentContext( context );
        // add error checking here
        ITK_ABORT(errorCode, "Error CGLSetCurrentContext\n");
    }

    void makeCurrent() {
        CGLSetCurrentContext( context );
    }

    void lock(){
        CGLLockContext( context );
    }

    void unlock() {
        CGLUnlockContext( context );
    }

    ~GLContextWithoutWindow() {
        CGLSetCurrentContext( NULL );
        CGLDestroyContext( context );
    }

};

void signal_handler(int signal) {
    printf("   ****************** signal_handler **********************\n");
    Platform::Thread::getMainThread()->interrupt();
}

int main(int argc, char* argv[]){

    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

    Platform::Signal::Set(signal_handler);
    ITKCommon::Path::setWorkingPath(ITKCommon::Path::getExecutablePath(argv[0]));
    // initialize self referencing of the main thread.
    Platform::Thread::staticInitialization();

    int num_of_system_threads = Platform::Thread::QueryNumberOfSystemThreads();
    
    GLContextWithoutWindow *ogl = new GLContextWithoutWindow();
    SyphonOpenGLServer *syServer;
    syServer = [[SyphonOpenGLServer alloc] initWithName:@"aRibeiro Cam 01" context:CGLGetCurrentContext() options:nil];


    //ogl->makeCurrent();
    //glViewport(0,0,1920,1080);

    YUV2RGB_Multithread m_YUV2RGB_Multithread( Platform::Thread::QueryNumberOfSystemThreads()/2, Platform::Thread::QueryNumberOfSystemThreads()*2 );
    Platform::IPC::LowLatencyQueueIPC yuy2_queue( "aRibeiro Cam 01", Platform::IPC::QueueIPC_READ, 8, 1920 * 1080 * 2);
    Platform::ObjectBuffer data_buffer;
    Platform::ObjectBuffer aux_rgb_buffer;
    data_buffer.setSize(1920 * 1080 * 2);
    aux_rgb_buffer.setSize(1920 * 1080 * 4);

    //GL_TEXTURE_2D
    openglWrapper::GLTexture *texture = new openglWrapper::GLTexture(GL_TEXTURE_2D, 1920, 1080, GL_RGBA);
    {
        std::vector<uint8_t> data(1920 * 1080 * 4, 255);
        texture->uploadBufferRGBA_8888(&data[0], 1920, 1080, false);
        //texture->writeToPNG("out.png");
    }

    //texture->active(0);
    //texture->deactive(0);
    
    /*
    [syServer publishFrameTexture:texture->mTexture 
              textureTarget:GL_TEXTURE_2D 
              imageRegion:NSMakeRect(0,0,1920,1080)
              textureDimensions:NSMakeSize(1920,1080)
              flipped:false];
              */

    
    if ([syServer bindToDrawFrameOfSize:NSMakeSize(1920,1080)]){
        //glViewport(0,0,1920,1080);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        [syServer unbindAndPublish];
    }

    while (!Platform::Thread::isCurrentThreadInterrupted()) {
        
        if (yuy2_queue.read(&data_buffer)) {
            int size_check = 1920 * 1080 * 2;
            if (data_buffer.size != size_check)
                continue;
            if (aux_rgb_buffer.size != 1920 * 1080 * 4) {
                aux_rgb_buffer.setSize(1920 * 1080 * 4);
                memset(aux_rgb_buffer.data, 255, 1920 * 1080 * 4);
            }

            uint8_t* in_buffer = (uint8_t*)data_buffer.data;
            uint8_t* out_buffer = (uint8_t*)aux_rgb_buffer.data;
            
            m_YUV2RGB_Multithread.yuy2_to_rgba(in_buffer, out_buffer, 1920, 1080);

            //ogl->makeCurrent();
            
            //ogl->lock();
            texture->uploadBufferRGBA_8888(out_buffer, 1920, 1080, false);
            //ogl->unlock();

            /*
            if ([syServer bindToDrawFrameOfSize:NSMakeSize(1920,1080)]){

                static float v = 0.0f;
                glViewport(0,0,1920,1080);
                v = fmod(v + 0.01f, 1.0f);
                glClearColor(0.0f, v, 1.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);

                [syServer unbindAndPublish];

                printf("Direct Draw OK\n");
            }
            */
            glFlush();

            [syServer publishFrameTexture:texture->mTexture 
                      textureTarget:GL_TEXTURE_2D
                      imageRegion:NSMakeRect(0,0,1920,1080)
                      textureDimensions:NSMakeSize(1920,1080)
                      flipped:false];

        }
    }

    [syServer stop];
    [syServer release];
    delete texture;
    delete ogl;
    [pool drain];

    return 0;
}
