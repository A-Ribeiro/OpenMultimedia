package goodapps.camerastreamer.gl;

import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.util.Log;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import goodapps.camerastreamer.camera.CameraHelper;

/**
 * Created by alessandro on 22/05/2017.
 */

public class GLRendererCamera implements GLSurfaceView.Renderer {

    static final String TAG = "GLRendererCamera";

    GLView mGLView;
    GLShaderRGB shader;
    GLTexture textureRGB;
    GLSquare square;
    float width;
    float height;

    Boolean initialized = false;

    public GLRendererCamera(GLView mGLView) {
        this.mGLView = mGLView;
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        GLES20.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        GLES20.glDisable( GLES20.GL_DEPTH_TEST );
        GLHelper.checkGLError("GL_DEPTH_TEST");

        textureRGB = new GLTexture(true);
        shader = new GLShaderRGB();
        square = new GLSquare();

        shader.EnableShader();
        shader.setTextureRGB(0);

        CameraHelper.setGLView( mGLView, textureRGB.mTexture );

        initialized = false;
    }

    public Boolean isInitialized() {
        return initialized;
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {

        Log.w(TAG,"onSurfaceChanged w: " + width + " h: " + height);

        GLES20.glViewport(0, 0, width, height);
        this.width = (float)width;
        this.height = (float)height;

        setupSquareScale(last_square_w,last_square_h);

        initialized = true;

    }

    @Override
    public void onDrawFrame(GL10 gl) {
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
        //if (textureY.isInitialized())
        {
            shader.EnableShader();
            textureRGB.active(0);
            square.Draw(shader);
        }
    }


    int last_square_w = 1;
    int last_square_h = 1;

    public void setupSquareScale(int w, int h) {

        last_square_w = w;
        last_square_h = h;

        float aspect_render = width/height;
        float aspect_texture = (float)w/(float)h;

        float scaleX = aspect_texture/aspect_render;
        float scaleY = 1.0f;

        if (aspect_render > aspect_texture){
            //fit width
            float zoom = 2.0f/scaleX;
            scaleX *= zoom;
            scaleY *= zoom;
        } else {
            //fit height
            float zoom = 2.0f/scaleY;
            scaleX *= zoom;
            scaleY *= zoom;
        }


        shader.EnableShader();
        shader.setScale(scaleX,scaleY);

    }

}