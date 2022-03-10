package goodapps.camerastreamer.camera;

import android.annotation.SuppressLint;
import android.content.Context;
//import android.graphics.Camera;
import android.graphics.ImageFormat;
import android.graphics.SurfaceTexture;

// old API to access camera
//import android.hardware.Camera;

// new API to access camera
import android.hardware.camera2.*;

import android.hardware.camera2.params.StreamConfigurationMap;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import androidx.annotation.RequiresApi;
import goodapps.camerastreamer.gl.GLRendererCamera;
import goodapps.camerastreamer.gl.GLView;

/**
 * Created by alessandro on 19/05/2017.
 */

@SuppressWarnings("deprecation")
public class CameraHelper {

    static final String TAG = "CameraHelper";

    public static final Object _syncObj = new Object();

    private static CameraReference mCamera = null;
    private static volatile SurfaceTexture surfaceTexture;

    private static HandlerThread surfaceUpdateThread = null;

    public static void setSurfaceTextureSize(int w, int h) {

        surfaceTexture.setDefaultBufferSize(
            w,h
        );
        /*
        if (glView != null) {
            glView.queueEvent(new Runnable() {
                @Override
                public void run() {
                    synchronized (_syncObj) {
                        surfaceTexture.updateTexImage();
                    }
                }
            });
        }
         */
    }

    private static volatile GLView glView = null;
    private static int textureID = 10;
//    private static CameraReference.Size size;

    public static boolean CanUseNewCameraAPI () {
        return Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP;
    }

    public static void setGLView(GLView glViewp, int textureIDp) {

        //create preview again if surface is created
        if (isOpened()){
            synchronized (mCamera) {
                CameraReference.PreviewCallback callback = mCamera.callback_thiz;

                stopPreview();

                synchronized (_syncObj) {
                    textureID = textureIDp;
                    glView = glViewp;
                    SetupRenderTexture(mCamera);
                }

                startPreview(callback);
            }
        }
        else
        {
            textureID = textureIDp;
            glView = glViewp;
        }
    }

    volatile static boolean textureProcessing;
    static class TextureAvailableAdapter implements SurfaceTexture.OnFrameAvailableListener{
        CameraReference camera = null;
        CameraReference.Size size = new CameraReference.Size(32,32);
        public TextureAvailableAdapter(CameraReference camera) {
            this.camera = camera;
            textureProcessing = false;
        }

        public void onFrameAvailable(SurfaceTexture surfaceTexture) {
            //misterious code...
            //if (textureProcessing)
                //return;

            //Log.w (TAG, "Setting preview texture size: "+size.width + "x" + size.height);
            textureProcessing = true;
            glView.queueEvent(
                new Runnable() {
                    @Override
                    public void run() {
                        synchronized (_syncObj) {
                            surfaceTexture.updateTexImage();

                            //synchronized (camera) {
                                //if (camera.isPreviewing()) {
                                    size = camera.getSizeForTexture();
                                    ((GLRendererCamera) glView.getRenderer()).setupSquareScale(size.width, size.height);
                                //}
                            //}

                            glView.requestRender();
                            textureProcessing = false;
                        }
                    }
                }
            );

            //Log.w(TAG, "onFrameAvailable");

            /*
            synchronized (_syncObj) {
                if (textureProcessing)
                    return;
            }
            */

            /*
            if (glView != null) {
                textureProcessing = true;

                glView.queueEvent(new Runnable() {
                    @Override
                    public void run() {

                        synchronized (_syncObj) {
                            if (surfaceTexture != null && glView != null) {

                                // setting the preview texture size
                                CameraReference.Size size = camera.getSizeForTexture();
                                //Log.w (TAG, "Setting preview texture size: "+size.width + "x" + size.height);

                                surfaceTexture.updateTexImage();
                                ((GLRendererCamera) glView.getRenderer()).setupSquareScale(size.width, size.height);
                                glView.requestRender();
                            }

                            textureProcessing = false;
                        }
                    }
                });
            }
             */


            //surfaceTexture.updateTexImage();
        }
    }

    static void SetupRenderTexture(CameraReference camera) {
        Log.w(TAG,"SetupRenderTexture... textureID: " + textureID);

        try {
            surfaceTexture = new SurfaceTexture(textureID);

            /*
            CameraReference.Size size = getSizeForTexture();
            surfaceTexture.setDefaultBufferSize(
                    size.width,
                    size.height
            );
            */

            if (CameraHelper.CanUseNewCameraAPI()) {
                if (surfaceUpdateThread == null) {
                    surfaceUpdateThread = new HandlerThread("surface texture image read thread", Thread.NORM_PRIORITY);
                    surfaceUpdateThread.start();
                }
                surfaceTexture.setOnFrameAvailableListener(new TextureAvailableAdapter(camera), new Handler(surfaceUpdateThread.getLooper()));
            } else {
                surfaceTexture.setOnFrameAvailableListener(new TextureAvailableAdapter(camera));
            }

            //surfaceTexture.setDefaultBufferSize(1920,1080);

            mCamera.setPreviewTexture(surfaceTexture);

            //set current size
            /*
            if (glView != null)
                glView.queueEvent(new Runnable() {
                    @Override
                    public void run() {
                        synchronized (_syncObj) {
                            if (surfaceTexture != null && glView != null) {

                                // setting the preview texture size
                                CameraReference.Size size = camera.getSizeForTexture();
                                Log.w (TAG, "[SetupRenderTexture] Setting preview texture size: "+size.width + "x" + size.height);

                                surfaceTexture.updateTexImage();
                                ((GLRendererCamera) glView.getRenderer()).setupSquareScale(size.width, size.height);
                                glView.requestRender();

                                //surfaceTexture.updateTexImage();
                            }
                        }
                    }
                });
            */

            //postSurfaceUpdate();

        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    /*
    public static Camera getCamera(){
        return mCamera;
    }
    */

    @SuppressWarnings("deprecation")
    public static int getCameraOrientation(Context context, final int cameraId) {
        if (CanUseNewCameraAPI()) {
            try {
                CameraManager manager = (CameraManager) context.getSystemService(Context.CAMERA_SERVICE);
                String[] cameraIds = manager.getCameraIdList();
                CameraCharacteristics characteristics = manager.getCameraCharacteristics(cameraIds[cameraId]);
                return characteristics.get(CameraCharacteristics.SENSOR_ORIENTATION);
            } catch (Exception e) { // CameraAccessException
                // TODO handle error properly or pass it on
                return 0;
            }
        } else {
            android.hardware.Camera.CameraInfo info = new android.hardware.Camera.CameraInfo();
            android.hardware.Camera.getCameraInfo(cameraId, info);
            return info.orientation;
        }
    }

    public static boolean OpenCamera(Context context, int id, Boolean wait_gl_view) {

        textureProcessing = false;

        if (wait_gl_view) {
            // wait glView
            long start = System.currentTimeMillis();
            long finish = System.currentTimeMillis();
            //30 seconds
            while (glView == null && (finish - start) < 30000) {
                try {
                    Thread.sleep(100);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                finish = System.currentTimeMillis();
            }
        }


        //GLRendererCamera renderer = (GLRendererCamera)glView.getRenderer();




        Log.w(TAG,"OpenCamera...");

        mCamera = CameraReference.open(context, id);

        if (wait_gl_view && mCamera != null) {
            SetupRenderTexture(mCamera);
        }

        return (mCamera != null);
    }

    public static void CloseCamera() {
        if (!isOpened())
            return;

        Log.w(TAG,"CloseCamera...");

        if (CameraHelper.CanUseNewCameraAPI()) {
            if (surfaceUpdateThread != null)
                surfaceUpdateThread.quitSafely();
        }

        surfaceUpdateThread = null;

        if (surfaceTexture != null) {
            surfaceTexture.setOnFrameAvailableListener(null);
            //surfaceTexture.release();
        }
        synchronized (_syncObj) {
            surfaceTexture = null;
        }

        try {
            while (textureProcessing)
                Thread.sleep(1);
        } catch (InterruptedException e) {
            e.printStackTrace();
            // we dont have the thread exit control...
            // pass the interrupt to the next handler
            Thread.currentThread().interrupt();
        }

        if (isPreviewing())
            stopPreview();

        try {
            mCamera.setPreviewTexture(null);
        } catch (IOException e) {
            e.printStackTrace();
        }

        mCamera.release();
        mCamera = null;

    }

    /**
     * Needs to call this before everything in the code related to the camera,
     * to avoid conflict with the current OpenGL viewing creation.
     */
    public static void onCreateResetGLView() {
        glView = null;
        textureID = 10;
    }

    public static boolean isOpened() {
        return (mCamera != null);
    }

    public static void configureCamera(int width, int height, boolean flash, int[] fpsRange, int imageFormat) {
        if (!isOpened() || isPreviewing() )
            return;

        mCamera.setParameters(width,height,flash,fpsRange, imageFormat);
    }

    public static CameraReference.Size getSizeForTexture() {
        if (!isOpened() || isPreviewing() )
            return null;

        return mCamera.getSizeForTexture();
    }

    //static List<byte[]> cameraReadedBuffers = new ArrayList<byte[]>();

    //volatile static Camera.PreviewCallback callback_thiz = null;
    volatile static boolean processing;

    public static void startPreview(CameraReference.PreviewCallback callback) {
        if (!isOpened() || isPreviewing() || callback == null)
            return;

        mCamera.startPreview(callback);
    }

    public static void stopPreview() {
        if (!isOpened() || !isPreviewing() )
            return;

        mCamera.stopPreview();

    }

    public static boolean isPreviewing() {
        return mCamera.isPreviewing();
    }

    @SuppressWarnings("deprecation")
    public static int getCameraCount(Context context) {
        if ( CanUseNewCameraAPI() ){
            CameraManager manager = (CameraManager) context.getSystemService(Context.CAMERA_SERVICE);
            String[] cameraIds = new String[0];
            try {
                cameraIds = manager.getCameraIdList();
            } catch (Exception e) {
                e.printStackTrace();
            }
            return cameraIds.length;
        } else {
            return android.hardware.Camera.getNumberOfCameras();
        }
    }

    public static void getCameraResolutionAndFPS(Context context, int cameraID, List<CameraReference.Size> res, List<int[]> fps, int imageFormat ) {
        res.clear();
        fps.clear();

        if (CanUseNewCameraAPI()) {

            try {
                CameraManager manager = (CameraManager) context.getSystemService(Context.CAMERA_SERVICE);
                String[] cameraIds = manager.getCameraIdList();
                CameraCharacteristics characteristics = manager.getCameraCharacteristics( cameraIds[cameraID] );
                StreamConfigurationMap map = characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
                android.util.Size []size_list = map.getOutputSizes(imageFormat);
                for(android.util.Size el :size_list ){
                    // force return just the fullhd images...
                    if (el.getWidth() <= 1920 && el.getHeight() <= 1080)
                        res.add(new CameraReference.Size(el.getWidth(), el.getHeight()));
                }

                fps.add(new int[]{30*1000,30*1000});
            } catch (Exception e) {
                e.printStackTrace();
            }


        } else {
            if (OpenCamera(context,cameraID, false)){
                mCamera.getCameraResolutionAndFPS(res, fps, imageFormat);
                CloseCamera();
            }
        }
    }


    public static CameraReference.Size getCurrentSize() {
        return new CameraReference.Size( mCamera.param_width, mCamera.param_height  );
    }

}
