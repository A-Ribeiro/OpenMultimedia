package goodapps.camerastreamer.gl;

import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.util.Log;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

//import dll_wrapper.XyphPackWrapper;

/**
 * Created by alessandro on 17/05/2017.
 */

public class GLRendererYUV2RGB implements GLSurfaceView.Renderer {

    GLView mGLView;

    volatile boolean initialized = false;

    GLShaderYUV2RGB shader;
    GLTexture textureY;
    GLTexture textureU;
    GLTexture textureV;

    GLSquare square;
    GLSphere sphere;

    float width;
    float height;

    public GLRendererYUV2RGB(GLView mGLView) {
        this.mGLView = mGLView;
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        GLES20.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        GLES20.glDisable( GLES20.GL_DEPTH_TEST );
        GLHelper.checkGLError("GL_DEPTH_TEST");

        textureY = new GLTexture(false);
        textureU = new GLTexture(false);
        textureV = new GLTexture(false);
        shader = new GLShaderYUV2RGB();
        square = new GLSquare();
        sphere = new GLSphere(5,5,1.0f);

        shader.EnableShader();
        shader.setTextureY(0);
        shader.setTextureU(1);
        shader.setTextureV(2);

        initialized = true;
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        GLES20.glViewport(0, 0, width, height);
        this.width = (float)width;
        this.height = (float)height;
        setupSquareScale(1,1);
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
        if (textureY.isInitialized()) {
            shader.EnableShader();
            textureY.active(0);
            textureU.active(1);
            textureV.active(2);
            square.Draw(shader);
        }
    }

    public void setupSquareScale(int w, int h) {

        float aspectW = width/height;
        float aspectH = 1.0f;

        if (aspectW > aspectH){
            aspectW = 1.0f;
            aspectH = height/width;
        }

        aspectW = 1.0f/aspectW;
        aspectH = 1.0f/aspectH;

        //check the inner width and height
        if (aspectW > aspectH)
            aspectW *= (float)w/(float)h;
        else
            aspectH *= (float)h/(float)w;

        //normalize
        /*
        if (aspectW > aspectH){
            aspectH = 2.0f / aspectW;
            aspectW = 2.0f;
        } else {
            aspectW = 2.0f / aspectH;
            aspectH = 2.0f;
        }
        */

        if (aspectW > aspectH) {
            aspectH = 2.0f * aspectH / aspectW;
            aspectW = 2.0f;
        } else {
            aspectW = 2.0f * aspectW / aspectH;
            aspectH = 2.0f;
        }


        shader.EnableShader();
        shader.setScale(aspectW,aspectH);
    }

    volatile Object processingFrame = null;
    //comes from another thread
    public void postYUV420Buffer(Object frame){ //XyphPackWrapper.YUV420Buffer

        if (!initialized)
            return;

        if (processingFrame != null)
            return;

        processingFrame = frame;
        mGLView.queueEvent(new Runnable() {
            @Override
            public void run() {

                /*
                setupSquareScale(processingFrame.width,processingFrame.height);

                textureY.uploadBufferAlpha8( processingFrame.y, processingFrame.width, processingFrame.height,processingFrame.width, 0 );
                textureU.uploadBufferAlpha8( processingFrame.u, processingFrame.width >> 1,processingFrame.height >> 1,processingFrame.width >> 1, 0 );
                textureV.uploadBufferAlpha8( processingFrame.v, processingFrame.width >> 1,processingFrame.height >> 1,processingFrame.width >> 1, 0 );
                 */


                mGLView.requestRender();
                processingFrame = null;
            }
        });
        try {
            while (processingFrame != null)
                Thread.sleep(1);
        } catch (InterruptedException e) {
            e.printStackTrace();
            // we dont have the thread exit control...
            // pass the interrupt to the next handler
            Thread.currentThread().interrupt();
        }
    }

    volatile byte[] YV12Frame = null;
    volatile int YV12width;
    volatile int YV12height;
    public void postYV12(byte[] frame, int width, int height){

        if (!initialized)
            return;

        //skip post frame, if the buffer still is in processing
        if (YV12Frame != null)
            return;

        YV12Frame = frame;
        YV12width = width;
        YV12height = height;

        mGLView.queueEvent(new Runnable() {
            @Override
            public void run() {
                setupSquareScale(YV12width,YV12height);

                int w_aligned_16 = (YV12width / 2) * 2;//((int)((float)init.param_width / 16.0 + 0.5)) * 16;
                int h_aligned_16 = (YV12height / 2) * 2;// ((int)((float)init.param_height / 16.0 + 0.5)) * 16;
                int h_uv_aligned_16 = h_aligned_16 / 2;// ((int)(((float)h_aligned_16 / 2.0) / 16.0 + 0.5)) * 16;

                int yStride = w_aligned_16;
                int uvStride = yStride/2;// ((int)(((float)yStride / 2.0) / 16.0 + 0.5)) * 16;
                int ySize = yStride * h_aligned_16;
                int uvSize = uvStride * h_uv_aligned_16;

                int size = ySize + uvSize * 2;

                /*
                int yStride   =  ((int)( (float)YV12width / 16.0 + 0.5 ) ) * 16;
                int uvStride  = ((int)( ((float)yStride / 2.0) / 16.0  + 0.5 )) * 16;
                int ySize     = yStride * YV12height;
                int uvSize    = uvStride * YV12height / 2;
                //int yRowIndex = yStride * y;
                //int uRowIndex = ySize + uvSize + uvStride * c;
                //int vRowIndex = ySize + uvStride * c;
                int size      = ySize + uvSize * 2;
                */

                textureY.uploadBufferAlpha8( YV12Frame, YV12width, YV12height,yStride, 0 ); // no copy buffer (offset 0)
                textureU.uploadBufferAlpha8( YV12Frame, YV12width >> 1,YV12height >> 1,uvStride,  ySize ); // copy buffer (offset != 0)
                textureV.uploadBufferAlpha8( YV12Frame, YV12width >> 1,YV12height >> 1, uvStride, ySize + uvSize); // copy buffer (offset != 0)

                mGLView.requestRender();

                YV12Frame = null;
            }
        });
    }

}
