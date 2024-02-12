package goodapps.camerastreamer.gl;

import android.graphics.Rect;
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
    GLShaderColor shaderColor;
    GLTexture textureRGB;
    GLSquare square;
    float width = 1.0f;
    float height = 1.0f;

    Boolean initialized = false;

    GLRendererCamera.Vec2 point = new Vec2(0,0);
    GLRendererCamera.Vec2 pointToSet = new Vec2(0,0);

    GLRendererCamera.Vec2 sensorArea = new Vec2(0,0);
    GLRendererCamera.Vec2 percentSensorArea = new Vec2(0,0);

    long old_time_ms = System.currentTimeMillis();
    long acc_ms = 99999999;

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
        shaderColor =  new GLShaderColor();
        square = new GLSquare();

        shader.EnableShader();
        shader.setTextureRGB(0);

        shaderColor.EnableShader();
        shaderColor.setColor(1.0f,0.0f,1.0f,0.5f);
        shaderColor.setSecondScale(0.02f, 0.02f);
        shaderColor.setSecondPoint(0.0f, 0.0f);

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

        //GLES20.glEnable(GLES20.GL_BLEND);
        GLES20.glBlendFunc(GLES20.GL_SRC_ALPHA, GLES20.GL_ONE_MINUS_SRC_ALPHA);
        GLES20.glBlendEquation(GLES20.GL_FUNC_ADD);

        initialized = true;

    }

    @Override
    public void onDrawFrame(GL10 gl) {
        long ANIM_MSEC = 500;
        long new_time_ms = System.currentTimeMillis();
        long delta_ms = new_time_ms - old_time_ms;
        if (acc_ms < ANIM_MSEC){
            acc_ms += delta_ms;
        } else
            acc_ms = ANIM_MSEC;
        old_time_ms = new_time_ms;


        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
        //if (textureY.isInitialized())
        {
            shader.EnableShader();
            textureRGB.active(0);
            square.Draw(shader);

            if (pointToSet.x != point.x || pointToSet.y != point.y) {
                point = pointToSet;
                acc_ms = 0;

                //Log.i(TAG, "x: " + point.x + " y: " + point.y);
                //Log.i(TAG, "scaleX: " + shaderColor.scaleX + " scaleY: " + shaderColor.scaleY);
                //shaderColor.setSecondPoint(((point.x - 0.5f) * 2.0f) / shaderColor.scaleX,(( - point.y + 0.5f) * 4.0f) / shaderColor.scaleY);
            }

            if (acc_ms < ANIM_MSEC) {
                GLES20.glEnable(GLES20.GL_BLEND);
                float acc_lrp_float = 1.0f - (float)acc_ms / (float)ANIM_MSEC;
                shaderColor.EnableShader();

                shaderColor.setSecondScale(acc_lrp_float * percentSensorArea.x, acc_lrp_float * percentSensorArea.y );

                shaderColor.setSecondPoint(((point.x - 0.5f)) * shaderColor.scaleX,(( - point.y + 0.5f) ) * shaderColor.scaleY);

                square.Draw(shader);
                GLES20.glDisable(GLES20.GL_BLEND);
            }
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
            scaleX = 2.0f;
            scaleY *= zoom;
        } else {
            //fit height
            float zoom = 2.0f/scaleY;
            scaleX *= zoom;
            scaleY = 2.0f;
        }


        shader.EnableShader();
        shader.setScale(scaleX,scaleY);

        shaderColor.EnableShader();
        shaderColor.setScale(scaleX,scaleY);

    }

    public static class Vec2 {
        public float x;
        public float y;
        public Vec2(float x, float y) {
            this.x = x;
            this.y = y;
        }
    }

    public Vec2 viewToImageCoord(Vec2 xy){
        //Log.i(TAG, "viewToImageCoord " + xy.x + ", " + xy.y);
        float aspect_render = width/height;
        float aspect_texture = (float)last_square_w/(float)last_square_h;

        float scaleX = aspect_render/aspect_texture;
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

        xy.x = xy.x - 0.5f;
        xy.y = xy.y - 0.5f;
        //Log.i(TAG, "   -> to range " + xy.x + ", " + xy.y);

        xy.x *= scaleX;
        xy.y *= scaleY;

        xy.x = xy.x * 0.5f + 0.5f;
        xy.y = xy.y * 0.5f + 0.5f;
        //Log.i(TAG, "   -> divide " + xy.x + ", " + xy.y);

        //xy.x *= last_square_w;
        //xy.y *= last_square_h;

        //xy.x *= scaleX;
        //xy.y *= scaleY;

        //scaleX /= aspect_render;

        return xy;
    }

    public void showSquare(GLRendererCamera.Vec2 point, Rect _sensorArea) {
        pointToSet = point;

        sensorArea.x =  _sensorArea.width();
        sensorArea.y =  _sensorArea.height();

        percentSensorArea.x = 300.0f / sensorArea.x;
        percentSensorArea.y = 300.0f / sensorArea.y;


        //percentSensorArea.x /= (float) last_square_w;
        //WpercentSensorArea.y /= (float) last_square_h;



    }

}