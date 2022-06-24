package goodapps.camerastreamer.camera;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.ImageFormat;
import android.graphics.SurfaceTexture;
// old API to access camera
//import android.hardware.Camera;
// new API to access camera
import android.hardware.camera2.*;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.media.Image;
import android.media.ImageReader;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.view.Surface;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import androidx.core.app.ActivityCompat;

import util.ObjectBuffer;
import util.ObjectPool;
import util.ObjectQueue;

/**
 * Created by alessandro on 19/05/2017.
 */

@SuppressWarnings("deprecation")
public class CameraReference {

    public static final int[] CaptureFormatPriority;
    private static Integer compatibleCameraImageFormat;

    static {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            CaptureFormatPriority = new int[]{
                    ImageFormat.YV12,
                    ImageFormat.NV21,
                    ImageFormat.YUV_420_888
            };
        } else {
            //old hardware... does not support ImageFormat.YUV_420_888
            CaptureFormatPriority = new int[]{
                    ImageFormat.YV12,
                    ImageFormat.NV21
            };
        }
        compatibleCameraImageFormat = null;
    }



    public static int chooseCompatibleCameraFormat(Context context, int cameraID) {

        if (compatibleCameraImageFormat != null)
            return compatibleCameraImageFormat;

        if (CameraHelper.CanUseNewCameraAPI()) {
            // New Camera API...
            try {
                CameraManager manager = (CameraManager) context.getSystemService(Context.CAMERA_SERVICE);
                String[] cameraIds = manager.getCameraIdList();
                CameraCharacteristics characteristics = manager.getCameraCharacteristics( cameraIds[cameraID] );
                StreamConfigurationMap map = characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);

                for(int format : CaptureFormatPriority) {
                    if (map.getOutputSizes(format) != null && map.getOutputSizes(format).length > 0) {
                        compatibleCameraImageFormat = format;
                        return format;
                    }
                }

            } catch (Exception e) {
                e.printStackTrace();
            }

        } else {
            // Old Camera API...
            compatibleCameraImageFormat = ImageFormat.YV12;
            return ImageFormat.YV12;
        }

        compatibleCameraImageFormat = ImageFormat.NV21;
        return ImageFormat.NV21;
    }

    //public static int DEFAULT_DATA_FORMAT = ImageFormat.JPEG;//very slow
    //public static final int DEFAULT_DATA_FORMAT = ImageFormat.YV12;//very fast
    //public static int DEFAULT_DATA_FORMAT = ImageFormat.YUV_420_888;
    //public static int DEFAULT_DATA_FORMAT = ImageFormat.YUV_420_888;//very fast

    static final String TAG = "CameraReference";

    //old api
    private android.hardware.Camera old_camera = null;
    private Thread old_PreviewThread = null;

    //private volatile Boolean old_has_buffer = false;
    //private volatile ObjectBuffer old_objectBuffer = null;

    private volatile ObjectQueue<ObjectBuffer> old_buffer_queue = new ObjectQueue<>();

    private HWBufferSize hwBufferSize = null;

    //new api
    private CameraDevice new_camera = null;
    private CameraManager new_manager = null;
    private String new_camera_name = "";
    private CameraCaptureSession cameraCaptureSession = null;
    private CaptureRequest.Builder captureRequestBuilder = null;
    private Context context = null;
    private ImageReader mImage = null;
    private Surface surface = null;
    private CameraCharacteristics characteristics;

    private HandlerThread auxiliaryThread = null;
    private HandlerThread imageReadThread = null;

    // preview callback
    public PreviewCallback callback_thiz = null;
    Object thiz = this;
    //public CameraReference.Size size = null;//new Size(32,32);

    // parameters
    int param_format = -1;
    int param_width = -1;
    int param_height = -1;
    boolean param_flash;
    int[] param_fpsRange;

    //private ByteBuffer byteBuffer = null;
    //private byte[] byteBuffer = null;

    public ObjectPool<ObjectBuffer> bufferPool = null;

    void reset_fields() {
        synchronized (CameraHelper._syncObj) {
            old_camera = null;
            //new api
            new_camera = null;
            new_manager = null;
            new_camera_name = "";
            cameraCaptureSession = null;
            captureRequestBuilder = null;
            context = null;
            mImage = null;
            surface = null;
            auxiliaryThread = null;
            imageReadThread = null;
        }
    }

    class HWBufferSize{
        int HW_ALIGN = 16;

        int hw_w_aligned_16;
        int hw_h_aligned_16;
        int hw_h_uv_aligned_16;

        int hw_yStride;
        int hw_uvStride;

        int hw_ySize;
        int hw_uvSize;

        int hw_size;

        int tcp_w_aligned_16;
        int tcp_h_aligned_16;
        int tcp_h_uv_aligned_16;

        int tcp_yStride;
        int tcp_uvStride;
        int tcp_ySize;
        int tcp_uvSize;

        int tcp_size;

        HWBufferSize(int width, int height, int alignment) {

            HW_ALIGN = alignment;// = 16

            hw_w_aligned_16 = width;
            hw_h_aligned_16 = height;
            hw_h_uv_aligned_16 = hw_h_aligned_16 / 2;

            hw_yStride = (hw_w_aligned_16 / HW_ALIGN + ((hw_w_aligned_16 % HW_ALIGN > 0) ? 1 : 0) ) * HW_ALIGN;
            hw_uvStride = hw_yStride / 2;
            hw_uvStride = (hw_uvStride / HW_ALIGN + ((hw_uvStride % HW_ALIGN > 0) ? 1 : 0) ) * HW_ALIGN;

            hw_ySize = hw_yStride * hw_h_aligned_16;
            hw_uvSize = hw_uvStride * hw_h_uv_aligned_16;

            hw_size = hw_ySize + hw_uvSize * 2;



            tcp_w_aligned_16 = (width / 2) * 2;//((int)((float)init.param_width / 16.0 + 0.5)) * 16;
            tcp_h_aligned_16 = (height / 2) * 2;// ((int)((float)init.param_height / 16.0 + 0.5)) * 16;
            tcp_h_uv_aligned_16 = tcp_h_aligned_16 / 2;// ((int)(((float)h_aligned_16 / 2.0) / 16.0 + 0.5)) * 16;

            tcp_yStride = tcp_w_aligned_16;
            tcp_uvStride = tcp_yStride / 2;// ((int)(((float)yStride / 2.0) / 16.0 + 0.5)) * 16;

            tcp_ySize = tcp_yStride * tcp_h_aligned_16;
            tcp_uvSize = tcp_uvStride * tcp_h_uv_aligned_16;

            tcp_size = tcp_ySize + tcp_uvSize * 2;

        }
    }

    static public CameraReference open(Context context, int id) {
        Log.w(TAG, "open");

        CameraReference result = new CameraReference();
        result.context = context;
        if (CameraHelper.CanUseNewCameraAPI()) {
            result.new_manager = (CameraManager) context.getSystemService(Context.CAMERA_SERVICE);
            try {
                String[] cameraIds = result.new_manager.getCameraIdList();
                result.new_camera_name = cameraIds[id];

                result.characteristics = result.new_manager.getCameraCharacteristics(result.new_camera_name);

                if (ActivityCompat.checkSelfPermission(context, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
                    result.release();
                    Log.w(TAG, "open - returning null");
                    return null;
                }

                result.auxiliaryThread = new HandlerThread("auxiliary thread", Thread.NORM_PRIORITY);
                result.auxiliaryThread.start();

                result.imageReadThread = new HandlerThread("image read thread", Thread.NORM_PRIORITY);
                result.imageReadThread.start();

                result.new_manager.openCamera(result.new_camera_name, new CameraInitializeAdapter(result), new Handler(result.auxiliaryThread.getLooper()));
            } catch (Exception e) {
                // TODO handle
                e.printStackTrace();
            }

            long start = System.currentTimeMillis();
            long finish = System.currentTimeMillis();
            //30 seconds
            while (result.new_camera == null && (finish - start) < 30000 ) {
                try {
                    Thread.sleep(100);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                finish = System.currentTimeMillis();
            }

            Log.w(TAG, "open - result.new_camera " + result.new_camera);

            if (result.new_camera == null) {
                result.release();
                Log.w(TAG, "open - returning null");
                return null;
            }


        }else {
            synchronized (CameraHelper._syncObj) {
                result.old_camera = android.hardware.Camera.open(id);
                if (result.old_camera == null) {
                    result.release();
                    Log.w(TAG, "open - returning null");
                    return null;
                }
            }
        }

        //result.size = result.getSize();

        Log.w(TAG, "open - returning camera reference");
        return result;
    }

    void release() {
        Log.w(TAG, "release");

        if (CameraHelper.CanUseNewCameraAPI()) {

            if (new_camera != null)
                new_camera.close();

            try {
                if (cameraCaptureSession != null)
                    cameraCaptureSession.close();
                cameraCaptureSession = null;
            } catch (Exception ex) {
                ex.printStackTrace();
            }

            if (mImage != null)
                mImage.close();

            Handler aux;
            aux = new Handler(auxiliaryThread.getLooper());

            long start = System.currentTimeMillis();
            long finish = System.currentTimeMillis();
            //30 seconds
            while (aux.hasMessages(0) && (finish - start) < 30000 ) {
                try {
                    Thread.sleep(100);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                finish = System.currentTimeMillis();
            }

            aux = new Handler(imageReadThread.getLooper());

            start = System.currentTimeMillis();
            finish = System.currentTimeMillis();
            //30 seconds
            while (aux.hasMessages(0) && (finish - start) < 30000 ) {
                try {
                    Thread.sleep(100);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                finish = System.currentTimeMillis();
            }


            if (auxiliaryThread != null)
                auxiliaryThread.quitSafely();

            if (imageReadThread != null)
                imageReadThread.quitSafely();

        } else {
            try {
                old_camera.setPreviewTexture(null);
            } catch (IOException e) {
                e.printStackTrace();
            }
            old_camera.release();

            //old_camera = null;
        }

        reset_fields();
    }


    void setParameters(int width, int height, boolean flash, int[] fpsRange, int imageFormat) {

        param_format = imageFormat;
        param_width = width;
        param_height = height;
        param_flash = flash;
        param_fpsRange = fpsRange;

        Log.w(TAG, "setParameters");
        if (CameraHelper.CanUseNewCameraAPI()){
            try {

                CameraReference.Size size = getSizeForTexture();
                CameraHelper.setSurfaceTextureSize(size.width,size.height);

                //CameraCharacteristics characteristics = new_manager.getCameraCharacteristics(new_camera_name);
                //StreamConfigurationMap map = characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);

                mImage = ImageReader.newInstance(width, height, imageFormat, 5);

                //new_manager.setTorchMode( new_camera_name,true);

            } catch (Exception e) {//CameraAccessException
                e.printStackTrace();
            }

        } else {
            android.hardware.Camera.Parameters params = old_camera.getParameters();

            //params.setPreviewFormat( ImageFormat.NV21 );
            params.setPreviewFormat( imageFormat ); // YUV 4:2:0 -- no interleaved as the NV21 is
            params.setPreviewFpsRange( fpsRange[0], fpsRange[1] );
            params.setPreviewSize(width, height);

            if (flash)
                params.setFlashMode( android.hardware.Camera.Parameters.FLASH_MODE_TORCH );
            else
                params.setFlashMode( android.hardware.Camera.Parameters.FLASH_MODE_OFF );

            params.setFocusMode( android.hardware.Camera.Parameters.FOCUS_MODE_CONTINUOUS_PICTURE );

            old_camera.setParameters(params);
        }

    }

    public Size getSizeForTexture() {

        //Log.w(TAG, "getSize");
        if (CameraHelper.CanUseNewCameraAPI()){

            if ( characteristics == null )
                return new Size(32,32);

            //int width = Resources.getSystem().getDisplayMetrics().widthPixels;
            //int height = Resources.getSystem().getDisplayMetrics().heightPixels;

            int width = param_width;
            int height = param_height;

            float aspect = (float)width/(float)height;

            float dst = Float.POSITIVE_INFINITY;
            int index = 0;

            try {
                //CameraCharacteristics characteristics = new_manager.getCameraCharacteristics(new_camera_name);
                StreamConfigurationMap map = characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);

                android.util.Size [] size_list = map.getOutputSizes(
                        SurfaceTexture.class
                        //surface.getClass()
                );

                int i=0;
                for (android.util.Size el:size_list) {
                    //Log.w(TAG, " " + el.getWidth() + " " + el.getHeight() );
                    float _aspect = (float)el.getWidth() / (float)el.getHeight();
                    float _dst = _aspect - aspect;
                    _dst = _dst * _dst;

                    if (_dst < dst ){
                        dst = _dst;
                        index = i;
                    }
                    i++;
                }

                return new Size(size_list[index].getWidth(),size_list[index].getHeight());

            } catch (Exception e) {
                e.printStackTrace();
            }

            return new Size(1920,1080);
        } else {
            if ( old_camera == null )
                return new Size(32,32);

            android.hardware.Camera.Parameters params = old_camera.getParameters();
            android.hardware.Camera.Size size = params.getPreviewSize();
            return new Size(size.width,size.height);
        }
    }

    public void setPreviewTexture(SurfaceTexture tex) throws IOException {
        Log.w(TAG, "setPreviewTexture");
        if (CameraHelper.CanUseNewCameraAPI()) {
            if (tex == null)
                surface = null;
            else
                surface = new Surface(tex);
        } else {
            old_camera.setPreviewTexture(tex);
        }
    }

    public void startPreview(PreviewCallback callback, ObjectPool<ObjectBuffer> _bufferPool) {

        bufferPool = _bufferPool;

        Log.w(TAG, "startPreview");
        if (CameraHelper.CanUseNewCameraAPI()) {

            try{
                captureRequestBuilder = new_camera.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
                captureRequestBuilder.addTarget(surface);
                captureRequestBuilder.addTarget(mImage.getSurface());

                {
                    captureRequestBuilder.set(CaptureRequest.COLOR_CORRECTION_MODE, CaptureRequest.COLOR_CORRECTION_MODE_FAST);
                    captureRequestBuilder.set(CaptureRequest.CONTROL_CAPTURE_INTENT, CaptureRequest.CONTROL_CAPTURE_INTENT_PREVIEW);
                    captureRequestBuilder.set(CaptureRequest.HOT_PIXEL_MODE, CaptureRequest.HOT_PIXEL_MODE_OFF);
                    captureRequestBuilder.set(CaptureRequest.EDGE_MODE, CaptureRequest.EDGE_MODE_OFF);
                    captureRequestBuilder.set(CaptureRequest.NOISE_REDUCTION_MODE, CaptureRequest.NOISE_REDUCTION_MODE_OFF);
                    captureRequestBuilder.set(CaptureRequest.TONEMAP_MODE, CaptureRequest.TONEMAP_MODE_FAST);
                    captureRequestBuilder.set(CaptureRequest.SHADING_MODE, CaptureRequest.SHADING_MODE_OFF);
                    captureRequestBuilder.set(CaptureRequest.CONTROL_VIDEO_STABILIZATION_MODE, CaptureRequest.CONTROL_VIDEO_STABILIZATION_MODE_OFF);
                }

                captureRequestBuilder.set(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE);
                if ( param_flash ) {
                    //captureRequestBuilder.set(CaptureRequest.CONTROL_AE_MODE, CaptureRequest.CONTROL_AE_MODE_ON_ALWAYS_FLASH);
                    //captureRequestBuilder.set(CaptureRequest.CONTROL_AE_MODE, CameraMetadata.CONTROL_AE_MODE_ON_ALWAYS_FLASH);
                    captureRequestBuilder.set(CaptureRequest.FLASH_MODE, CameraMetadata.FLASH_MODE_TORCH);
                }
                else {
                    //captureRequestBuilder.set(CaptureRequest.CONTROL_AE_MODE, CameraMetadata.CONTROL_AE_MODE_ON);
                    captureRequestBuilder.set(CaptureRequest.FLASH_MODE, CameraMetadata.FLASH_MODE_OFF);
                }


                //CaptureRequest.Builder captureBuilder = mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
                //captureBuilder.set(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE);
                //captureBuilder.set(CaptureRequest.CONTROL_AE_MODE, CaptureRequest.CONTROL_AE_MODE_ON_AUTO_FLASH);

                List<Surface> outputSurfaces = new ArrayList<>(2);
                outputSurfaces.add(surface);
                outputSurfaces.add(mImage.getSurface());


                //new_camera.createCaptureSession(Arrays.asList(surface),new SessionInitializationAdapter(this), new Handler(auxiliaryThread.getLooper()));
                new_camera.createCaptureSession(outputSurfaces,new SessionInitializationAdapter(this), new Handler(auxiliaryThread.getLooper()));

                //wait session...
                long start = System.currentTimeMillis();
                long finish = System.currentTimeMillis();
                //30 seconds
                while (cameraCaptureSession == null && (finish - start) < 30000 ) {
                    try {
                        Thread.sleep(100);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                    finish = System.currentTimeMillis();
                }

                Log.w(TAG, "SESSION INITIALIZED: "+cameraCaptureSession);

                //Toast.makeText(context, "Configuration change", Toast.LENGTH_SHORT).show();

                mImage.setOnImageAvailableListener(new ImageReaderAdapter(this), new Handler(imageReadThread.getLooper()));

                //CameraHelper.postSurfaceUpdate();

                synchronized (this) {
                    callback_thiz = callback;
                }

            } catch (Exception e) {
                e.printStackTrace();
            }

        } else {

            android.hardware.Camera.Parameters params = old_camera.getParameters();
            android.hardware.Camera.Size previewFrameSize = params.getPreviewSize();

            //size = new CameraReference.Size(previewFrameSize.width, previewFrameSize.height);

            //compute buffer size for NV21 or YV12
            hwBufferSize = new HWBufferSize( previewFrameSize.width, previewFrameSize.height, 16 );

            //if (byteBuffer == null || byteBuffer.length != hwBufferSize.tcp_size){
            //    //byteBuffer = ByteBuffer.allocateDirect(hwBufferSize.tcp_size);
            //    byteBuffer = new byte[hwBufferSize.tcp_size];
            //}
            //full size image 4bytes per pixel
            //int size = previewFrameSize.width * previewFrameSize.height * 4;

            //clear buffer queue
            old_camera.setPreviewCallbackWithBuffer(null);

            //add 5 buffers for the camera hardware read...
            for(int bufferCount = 0;bufferCount<5;bufferCount++)
                old_camera.addCallbackBuffer(new byte[hwBufferSize.hw_size]);

            synchronized (this) {
                callback_thiz = callback;
            }

            old_PreviewThread = new Thread(new Runnable() {
                @Override
                public void run() {
                    while (!Thread.interrupted()){
                        ObjectBuffer buffer = old_buffer_queue.dequeue();
                        if (buffer == null)//signal
                            break;
                        synchronized ( thiz ) {
                            if (isPreviewing())
                                callback_thiz.onPreviewFrame(buffer,old_camera);
                        }
                    }
                }
            }, "Camera Preview Thread");

            old_PreviewThread.start();

            old_camera.setPreviewCallbackWithBuffer(new android.hardware.Camera.PreviewCallback() {
                @Override
                public void onPreviewFrame(byte[] data, android.hardware.Camera camera) {

                    // allow max of 16 elementos in the queue
                    if (old_buffer_queue.size() < 16) {

                        ObjectBuffer objectBuffer = bufferPool.create();
                        objectBuffer.setSize(hwBufferSize.tcp_size);
                        byte[] byteBuffer = objectBuffer.getArray();

                        // Y
                        for( int h = 0; h < hwBufferSize.tcp_h_aligned_16; h++ )
                            System.arraycopy( data,h*hwBufferSize.hw_yStride,  byteBuffer,h*hwBufferSize.tcp_yStride, hwBufferSize.tcp_yStride );

                        // U
                        for( int h = 0; h < hwBufferSize.tcp_h_uv_aligned_16; h++ )
                            System.arraycopy(
                                    data,hwBufferSize.hw_ySize + hwBufferSize.hw_uvSize + h*hwBufferSize.hw_uvStride,
                                    byteBuffer,hwBufferSize.tcp_ySize + h*hwBufferSize.tcp_uvStride, hwBufferSize.tcp_uvStride );

                        // V
                        for( int h = 0; h < hwBufferSize.tcp_h_uv_aligned_16; h++ )
                            System.arraycopy(
                                    data,hwBufferSize.hw_ySize + h*hwBufferSize.hw_uvStride,
                                    byteBuffer,hwBufferSize.tcp_ySize + hwBufferSize.tcp_uvSize + h*hwBufferSize.tcp_uvStride, hwBufferSize.tcp_uvStride );

                        old_buffer_queue.enqueue(objectBuffer);
                    }

                    //add the buffer to the camera again...
                    camera.addCallbackBuffer(data);
                }
            });



/*
        callback_thiz = callback;
        mCamera.setPreviewCallback(new Camera.PreviewCallback() {
            @Override
            public void onPreviewFrame(byte[] data, Camera camera) {
                processing = true;
                callback_thiz.onPreviewFrame(data,camera);
                processing = false;
            }
        });
*/
            old_camera.startPreview();
        }
    }

    public void stopPreview() {
        Log.w(TAG, "stopPreview");
        if (CameraHelper.CanUseNewCameraAPI()) {

            try {
                if (cameraCaptureSession != null)
                    cameraCaptureSession.close();
                cameraCaptureSession = null;
            } catch (Exception ex) {
                ex.printStackTrace();
            }

            synchronized (this) {
                callback_thiz = null;
            }

        } else {

            old_camera.setPreviewCallback(null);

            /*
            try {
                while (processing){
                    Thread.sleep(1);
                }
            } catch (InterruptedException e) {
                // we dont have the thread exit control...
                // pass the interrupt to the next handler
                Thread.currentThread().interrupt();
            }
            */

            old_camera.stopPreview();

            synchronized (this) {
                callback_thiz = null;
            }

            old_PreviewThread.interrupt();
            //wait
            while (old_PreviewThread.isAlive()) { //|| old_has_buffer
                try {
                    Thread.sleep(100);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
            old_PreviewThread = null;

        }
    }

    public boolean isPreviewing() {
        synchronized(this) {
            if (CameraHelper.CanUseNewCameraAPI()) {
                return (callback_thiz != null);
            } else {
                return (callback_thiz != null);
            }
        }
    }


    public void getCameraResolutionAndFPS( List<CameraReference.Size> res, List<int[]> fps, int imageFormat ) {
        if (CameraHelper.CanUseNewCameraAPI()) {
            try {
                //CameraCharacteristics characteristics = new_manager.getCameraCharacteristics(new_camera_name);
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
            List<android.hardware.Camera.Size> size_list = old_camera.getParameters().getSupportedPreviewSizes();
            for(int i=0;i<size_list.size();i++) {
                android.hardware.Camera.Size el = size_list.get(i);
                res.add(new CameraReference.Size(el.width, el.height));
            }
            fps.addAll( old_camera.getParameters().getSupportedPreviewFpsRange() );
        }
    }






    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    static
    class CameraInitializeAdapter extends CameraDevice.StateCallback {
        CameraReference init;
        public CameraInitializeAdapter(CameraReference init){
            super();
            this.init = init;
        }

        @Override
        public void onOpened(@NonNull CameraDevice cameraDevice) {
            synchronized (CameraHelper._syncObj) {
                init.new_camera = cameraDevice;
            }
            Log.w(TAG, "camera ok...");
        }

        @Override
        public void onDisconnected(@NonNull CameraDevice cameraDevice) {
            synchronized (CameraHelper._syncObj) {
                init.new_camera = cameraDevice;
            }
            Log.w(TAG, "camera disconnected...");
        }

        @Override
        public void onError(@NonNull CameraDevice cameraDevice, int i) {
            synchronized (CameraHelper._syncObj) {
                init.new_camera = cameraDevice;
            }
            Log.w(TAG, "camera error...");
        }
    }

    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    static
    class SessionInitializationAdapter extends CameraCaptureSession.StateCallback {
        CameraReference init;
        public SessionInitializationAdapter(CameraReference init){
            super();
            this.init = init;
        }

        @Override
        public void onConfigured(@NonNull CameraCaptureSession cameraCaptureSession) {
            this.init.cameraCaptureSession = cameraCaptureSession;

            //updatePreview();
            //this.init.captureRequestBuilder.set(CaptureRequest.CONTROL_MODE, CameraMetadata.CONTROL_MODE_AUTO);
            try {
                cameraCaptureSession.setRepeatingRequest(this.init.captureRequestBuilder.build(), null, null);//mBackgroundHandler);
            } catch (Exception e) {
                e.printStackTrace();
            }

            Log.w(TAG, "SessionInitializationAdapter onConfigured");
        }

        @Override
        public void onConfigureFailed(@NonNull CameraCaptureSession cameraCaptureSession) {
            this.init.cameraCaptureSession = cameraCaptureSession;

            Log.w(TAG, "SessionInitializationAdapter onConfigureFailed");
        }
    }

    @RequiresApi(api = Build.VERSION_CODES.KITKAT)
    static
    class ImageReaderAdapter implements ImageReader.OnImageAvailableListener {

        CameraReference init;

        public ImageReaderAdapter(CameraReference init) {
            this.init = init;
        }

        @Override
        public void onImageAvailable(ImageReader imageReader) {
            Image frame = imageReader.acquireNextImage();
            int format = imageReader.getImageFormat();//frame.getFormat();
            if ( format == ImageFormat.YUV_420_888 ) {
                synchronized(init) {
                    if (!init.isPreviewing()){
                        frame.close();
                        return;
                    }

                    int w_aligned_16 = init.param_width;
                    int h_aligned_16 = init.param_height;
                    int h_uv_aligned_16 = h_aligned_16 >> 1;

                    int yStride = w_aligned_16;
                    int uvStride = yStride >> 1;
                    int ySize = yStride * h_aligned_16;
                    int uvSize = uvStride * h_uv_aligned_16;

                    int size = ySize + (uvSize << 1);

                    //byte[] data = new byte[size];
                    //if (init.byteBuffer == null || init.byteBuffer.length != size){
                    //    init.byteBuffer = new byte[size];
                    //}

                    ObjectBuffer objectBuffer = init.bufferPool.create();
                    objectBuffer.setSize(size);
                    byte[] byteBuffer = objectBuffer.getArray();

                    Image.Plane[] planes = frame.getPlanes();
                    boolean is888Buffer = planes[1].getRowStride() >= yStride;//planes[0].getRowStride();

                    if (init.param_format == ImageFormat.YUV_420_888 && is888Buffer) {

                        ByteBuffer cameraByteBuffer;
                        int data_row_stride;

                        cameraByteBuffer = planes[0].getBuffer();
                        data_row_stride = planes[0].getRowStride();
                        //cameraByteBuffer.clear();
                        for (int h = 0; h < h_aligned_16; h++) {
                            cameraByteBuffer.position(h * data_row_stride);
                            cameraByteBuffer.get(byteBuffer, h * yStride, yStride);
                        }

                        cameraByteBuffer = planes[1].getBuffer();
                        data_row_stride = planes[1].getRowStride();
                        //cameraByteBuffer.clear();
                        for (int h = 0; h < h_uv_aligned_16; h++) {
                            int aux = h * data_row_stride;
                            for (int x = 0; x < uvStride; x++) {
                                cameraByteBuffer.position(aux + (x << 1));
                                byteBuffer[ySize + h * uvStride + x] = cameraByteBuffer.get();
                            }
                        }

                        cameraByteBuffer = planes[2].getBuffer();
                        data_row_stride = planes[2].getRowStride();
                        //cameraByteBuffer.clear();
                        for (int h = 0; h < h_uv_aligned_16; h++) {
                            int aux = h * data_row_stride;
                            for (int x = 0; x < uvStride; x++) {
                                cameraByteBuffer.position(aux + (x << 1));
                                byteBuffer[ySize + uvSize + h * uvStride + x] = cameraByteBuffer.get();
                            }
                        }

                        init.callback_thiz.onPreviewFrame(objectBuffer, init.new_camera);
                    } else if (init.param_format == ImageFormat.YV12) {

                        ByteBuffer cameraByteBuffer;
                        int data_row_stride;

                        cameraByteBuffer = planes[0].getBuffer();
                        data_row_stride = planes[0].getRowStride();
                        //cameraByteBuffer.clear();
                        for (int h = 0; h < h_aligned_16; h++) {
                            cameraByteBuffer.position(h * data_row_stride);
                            cameraByteBuffer.get(byteBuffer, h * yStride, yStride);
                        }

                        cameraByteBuffer = planes[1].getBuffer();
                        data_row_stride = planes[1].getRowStride();
                        //cameraByteBuffer.clear();
                        for (int h = 0; h < h_uv_aligned_16; h++) {
                            cameraByteBuffer.position(h * data_row_stride);
                            cameraByteBuffer.get(byteBuffer, ySize + h * uvStride, uvStride);
                        }

                        cameraByteBuffer = planes[2].getBuffer();
                        data_row_stride = planes[2].getRowStride();
                        //cameraByteBuffer.clear();
                        for (int h = 0; h < h_uv_aligned_16; h++) {
                            cameraByteBuffer.position(h * data_row_stride);
                            cameraByteBuffer.get(byteBuffer, ySize + uvSize + h * uvStride, uvStride);
                        }

                        init.callback_thiz.onPreviewFrame(objectBuffer, init.new_camera);
                    }

                }
            } else if (format == ImageFormat.YV12){
                synchronized(init) {

                    if (!init.isPreviewing()){
                        frame.close();
                        return;
                    }

                    int w_aligned_16 = init.param_width;
                    int h_aligned_16 = init.param_height;
                    int h_uv_aligned_16 = h_aligned_16 >> 1;

                    int yStride = w_aligned_16;
                    int uvStride = yStride >> 1;
                    int ySize = yStride * h_aligned_16;
                    int uvSize = uvStride * h_uv_aligned_16;

                    int size = ySize + (uvSize << 1);

                    //byte[] data = new byte[size];
                    //if (init.byteBuffer == null || init.byteBuffer.length != size){
                    //    init.byteBuffer = new byte[size];
                    //}
                    ObjectBuffer objectBuffer = init.bufferPool.create();
                    objectBuffer.setSize(size);
                    byte[] byteBuffer = objectBuffer.getArray();

                    Image.Plane[] planes = frame.getPlanes();

                    ByteBuffer cameraByteBuffer;
                    int data_row_stride;

                    cameraByteBuffer = planes[0].getBuffer();
                    data_row_stride = planes[0].getRowStride();
                    //cameraByteBuffer.clear();
                    for (int h = 0; h < h_aligned_16; h++) {
                        cameraByteBuffer.position(h * data_row_stride);
                        cameraByteBuffer.get(byteBuffer, h * yStride, yStride);
                    }

                    cameraByteBuffer = planes[1].getBuffer();
                    data_row_stride = planes[1].getRowStride();
                    //cameraByteBuffer.clear();
                    for (int h = 0; h < h_uv_aligned_16; h++) {
                        cameraByteBuffer.position(h * data_row_stride);
                        cameraByteBuffer.get(byteBuffer, ySize + h * uvStride, uvStride);
                    }

                    cameraByteBuffer = planes[2].getBuffer();
                    data_row_stride = planes[2].getRowStride();
                    //cameraByteBuffer.clear();
                    for (int h = 0; h < h_uv_aligned_16; h++) {
                        cameraByteBuffer.position(h * data_row_stride);
                        cameraByteBuffer.get(byteBuffer, ySize + uvSize + h * uvStride, uvStride);
                    }

                    init.callback_thiz.onPreviewFrame(objectBuffer,init.new_camera);

                }
            } else if ( format == ImageFormat.NV21 ) {

                synchronized(init) {

                    if (!init.isPreviewing()){
                        frame.close();
                        return;
                    }

                    int w_aligned_16 = init.param_width;
                    int h_aligned_16 = init.param_height;
                    int h_uv_aligned_16 = h_aligned_16 >> 1;

                    int yStride = w_aligned_16;
                    int uvStride = yStride >> 1;
                    int ySize = yStride * h_aligned_16;
                    int uvSize = uvStride * h_uv_aligned_16;

                    int size = ySize + (uvSize << 1);

                    //byte[] data = new byte[size];
                    //if (init.byteBuffer == null || init.byteBuffer.length != size){
                    //    init.byteBuffer = new byte[size];
                    //}
                    ObjectBuffer objectBuffer = init.bufferPool.create();
                    objectBuffer.setSize(size);
                    byte[] byteBuffer = objectBuffer.getArray();

                    Image.Plane[] planes = frame.getPlanes();

                    ByteBuffer cameraByteBuffer;
                    int data_row_stride;

                    cameraByteBuffer = planes[0].getBuffer();
                    data_row_stride = planes[0].getRowStride();
                    //cameraByteBuffer.clear();
                    for (int h = 0; h < h_aligned_16; h++) {
                        cameraByteBuffer.position(h * data_row_stride);
                        cameraByteBuffer.get(byteBuffer, h * yStride, yStride);
                    }

                    boolean bothComponentsAreInPlane = planes[1].getRowStride() >= yStride;

                    if (bothComponentsAreInPlane) {
                        cameraByteBuffer = planes[1].getBuffer();
                        data_row_stride = planes[1].getRowStride();
                        //cameraByteBuffer.clear();
                        for (int h = 0; h < h_uv_aligned_16; h++) {
                            cameraByteBuffer.position(h * data_row_stride);
                            for (int x = 0; x < uvStride; x++) {
                                // V
                                byteBuffer[ySize + uvSize + h * uvStride + x] = cameraByteBuffer.get();
                                // U
                                byteBuffer[ySize + h * uvStride + x] = cameraByteBuffer.get();
                            }
                        }
                    } else {

                        cameraByteBuffer = planes[2].getBuffer();
                        data_row_stride = planes[2].getRowStride();
                        //cameraByteBuffer.clear();
                        for (int h = 0; h < h_uv_aligned_16; h++) {
                            cameraByteBuffer.position(h * data_row_stride);
                            cameraByteBuffer.get(byteBuffer, ySize + h * uvStride, uvStride);
                        }

                        cameraByteBuffer = planes[1].getBuffer();
                        data_row_stride = planes[1].getRowStride();
                        //cameraByteBuffer.clear();
                        for (int h = 0; h < h_uv_aligned_16; h++) {
                            cameraByteBuffer.position(h * data_row_stride);
                            cameraByteBuffer.get(byteBuffer, ySize + uvSize + h * uvStride, uvStride);
                        }
                    }

                    init.callback_thiz.onPreviewFrame(objectBuffer,init.new_camera);

                }

            }

            frame.close();
        }
    }

    public static int getCameraFacingBack(Context context) {
        if (CameraHelper.CanUseNewCameraAPI()) {

            CameraManager cameraManager = (CameraManager) context.getSystemService(Context.CAMERA_SERVICE);

            try {
                String[] cameraIds = cameraManager.getCameraIdList();
                for (int i = 0; i < cameraIds.length; i++) {
                    CameraCharacteristics characteristics = cameraManager.getCameraCharacteristics(cameraIds[i]);
                    if (CameraCharacteristics.LENS_FACING_BACK == characteristics.get(CameraCharacteristics.LENS_FACING)) {

                        Log.w(TAG, "Camera facing back:" + i);

                        return i;
                    }
                }
            } catch (Exception e) {
                e.printStackTrace();
            }

            return 0;
        } else {
            return android.hardware.Camera.CameraInfo.CAMERA_FACING_BACK;
        }
    }

    public interface PreviewCallback {
        void onPreviewFrame(ObjectBuffer objectBuffer, Object src_camera);
    }

    public static class Size {
        public int width;
        public int height;

        public Size(int w, int h) {
            width = w;
            height = h;
        }

        @Override
        public boolean equals(Object o) {
            if (o instanceof Size) {
                Size s = (Size) o;
                return s.width == width && s.height == height;
            }
            return false;
        }
    }


}