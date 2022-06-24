package goodapps.camerastreamer;

import android.Manifest;
import android.annotation.SuppressLint;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.zip.Deflater;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;

import androidx.core.app.ActivityCompat;
import goodapps.camerastreamer.camera.CameraHelper;
import goodapps.camerastreamer.camera.CameraReference;
import goodapps.camerastreamer.camera.ConfigurationData;
import goodapps.camerastreamer.codec.VideoEncoder;
import goodapps.camerastreamer.gl.GLRendererCamera;
import goodapps.camerastreamer.gl.GLView;

import network.NetworkConstants;
import network.StreamDiscovering;
import network.StreamTCPServer;
import util.ObjectBuffer;

/**
 * An example full-screen activity that shows and hides the system UI (i.e.
 * status bar and navigation/system bar) with user interaction.
 */
public class CameraStreamingGLViewActivity extends AppCompatActivity implements Runnable {


    public static final int FORMAT_YUV2 = 1;//Android default image format
    public static final int FORMAT_YUY2 = 2;//OBS channel image format
    public static final int FORMAT_YUV420P = 3;
    public static final int FORMAT_YUV420P_ZLIB = 100;
    public static final int FORMAT_H264 = 1000;
    public static final int FORMAT_HEVC = 1001;
    public static final int FORMAT_3GPP = 1002;
    public static final int FORMAT_VP9 = 1003;

    static final String TAG = "CameraStreamingGLView";

    CameraStreamingGLViewActivity thiz = this;
    volatile ConfigurationData configurationData = null;
    private GLView mGLView = null;


    volatile StreamDiscovering streamDiscovering = null;
    volatile StreamTCPServer streamTCPServer = null;
    volatile ByteBuffer byteBuffer = null;
    volatile byte[] auxCompressBuffer = null;


    /*
    volatile H264VideoEncoder h264VideoEncoder = null;
    volatile HEVCVideoEncoder hevcVideoEncoder = null;
    volatile _3GPPVideoEncoder _3gppVideoEncoder = null;
    volatile VP9VideoEncoder vp9VideoEncoder = null;
    */

    volatile VideoEncoder videoEncoder = null;



    /**
     * Whether or not the system UI should be auto-hidden after
     * {@link #AUTO_HIDE_DELAY_MILLIS} milliseconds.
     */
    private static final boolean AUTO_HIDE = true;

    /**
     * If {@link #AUTO_HIDE} is set, the number of milliseconds to wait after
     * user interaction before hiding the system UI.
     */
    private static final int AUTO_HIDE_DELAY_MILLIS = 3000;

    /**
     * Some older devices needs a small delay between UI widget updates
     * and a change of the status and navigation bar.
     */
    private static final int UI_ANIMATION_DELAY = 300;
    private final Handler mHideHandler = new Handler();
    //private View mContentView;
    private final Runnable mHidePart2Runnable = new Runnable() {
        @SuppressLint("InlinedApi")
        @Override
        public void run() {
            // Delayed removal of status and navigation bar

            // Note that some of these constants are new as of API 16 (Jelly Bean)
            // and API 19 (KitKat). It is safe to use them, as they are inlined
            // at compile-time and do nothing on earlier devices.
            if (mGLView != null)
            mGLView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_LOW_PROFILE
                    | View.SYSTEM_UI_FLAG_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                    | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                    | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION);
        }
    };
    //private View mControlsView;
    private final Runnable mShowPart2Runnable = new Runnable() {
        @Override
        public void run() {
            // Delayed display of UI elements
            /*
            ActionBar actionBar = getSupportActionBar();
            if (actionBar != null) {
                actionBar.show();
            }
            */
            //mControlsView.setVisibility(View.VISIBLE);
        }
    };
    private boolean mVisible;
    private final Runnable mHideRunnable = new Runnable() {
        @Override
        public void run() {
            hide();
        }
    };
    /**
     * Touch listener to use for in-layout UI controls to delay hiding the
     * system UI. This is to prevent the jarring behavior of controls going away
     * while interacting with activity UI.
     */
    private final View.OnTouchListener mDelayHideTouchListener = new View.OnTouchListener() {
        @Override
        public boolean onTouch(View view, MotionEvent motionEvent) {
        if (AUTO_HIDE) {
            delayedHide(AUTO_HIDE_DELAY_MILLIS);
        }
        return false;
        }
    };

    void initializeGLView() {
        if (mGLView != null)
            return;

        CameraHelper.onCreateResetGLView();

        mGLView = new GLView(this, GLRendererCamera.class);
        //mGLView = new GLView(this, GLRendererYUV2RGB.class);
        setContentView(mGLView);


        mGLView.setSystemUiVisibility( View.SYSTEM_UI_FLAG_LOW_PROFILE
                | View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION);

        // Set up the user interaction to manually show or hide the system UI.
        mGLView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                toggle();
            }
        });
    }


    //WifiManager.MulticastLock multicastLock = null;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        WifiManager manager = (WifiManager) thiz.getApplicationContext().getSystemService( Context.WIFI_SERVICE);
        //multicastLock = manager.createMulticastLock("CameraStreammer");
        //multicastLock.setReferenceCounted(true);

        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON, WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.hide();
        }

        initializeGLView();

        mVisible = false;
    }

    @Override
    protected void onPostCreate(Bundle savedInstanceState) {
        super.onPostCreate(savedInstanceState);

        // Trigger the initial hide() shortly after the activity has been
        // created, to briefly hint to the user that UI controls
        // are available.
        //delayedHide(100);
    }

    volatile byte[] processingdata;

    @Override
    protected void onResume(){
        super.onResume();

        Log.d(TAG,"onResume");

        //multicastLock.acquire();
        //contentStreamer.initializeAsViewer( null );//to enable auto server redirect based on the transmission channel

        configurationData = ConfigurationData.createFromByteArray( (byte[])getIntent().getExtras().get("ConfigurationData") );

        can_exit = false;
        Thread thread = new Thread(this, "Activity OnResume");
        thread.start();



    }
    private Boolean can_exit;

    @Override
    public void run() {

        // need to wait the GLView and SurfaceTexture to initialize before use it...
        int cameraID = CameraReference.getCameraFacingBack( this );
        if (CameraHelper.OpenCamera( this, cameraID, true )) {

            int captureImageFormat = CameraReference.chooseCompatibleCameraFormat(this, cameraID);

            CameraHelper.configureCamera( configurationData.width,configurationData.height, configurationData.useFlashLight ,new int[] {configurationData.fps_min,configurationData.fps_max }, captureImageFormat);

            streamTCPServer = StreamTCPServer.create( NetworkConstants.PUBLIC_PORT_START );
            // 1 sec interval...
            streamDiscovering = new StreamDiscovering(configurationData.deviceName, 1000);

            CameraReference.Size currentSize = CameraHelper.getCurrentSize();

            //compute buffer size for NV21 or YV12
            int w_aligned_16 = (currentSize.width / 2) * 2;//((int)((float)init.param_width / 16.0 + 0.5)) * 16;
            int h_aligned_16 = (currentSize.height / 2) * 2;// ((int)((float)init.param_height / 16.0 + 0.5)) * 16;
            int h_uv_aligned_16 = h_aligned_16 / 2;// ((int)(((float)h_aligned_16 / 2.0) / 16.0 + 0.5)) * 16;

            int yStride = w_aligned_16;
            int uvStride = yStride / 2;// ((int)(((float)yStride / 2.0) / 16.0 + 0.5)) * 16;
            int ySize = yStride * h_aligned_16;
            int uvSize = uvStride * h_uv_aligned_16;

            int size_yuv420 = ySize + uvSize * 2;

            int size_full_888_image = (currentSize.width * currentSize.height) * 3; //full size image

            if (byteBuffer == null || byteBuffer.capacity() != size_full_888_image + 16) {
                byteBuffer = ByteBuffer.allocateDirect(size_full_888_image + 16);
                byteBuffer.order(ByteOrder.LITTLE_ENDIAN);

                auxCompressBuffer = new byte[size_full_888_image + 16];
            }

            CameraHelper.startPreview(
                new CameraReference.PreviewCallback() {
                    @Override
                    public void onPreviewFrame(CameraReference cameraReference, ObjectBuffer objectBuffer, Object camera) {
                        if (streamTCPServer != null) {

                            if (size_yuv420 == objectBuffer.getSize()) {

                                VideoEncoder.Type selectedType = VideoEncoder.Type.None;
                                int transmission_format = 0;

                                if (configurationData.transmissionType.equals("HEVC (HW)")){
                                    selectedType = VideoEncoder.Type.HEVC;
                                    transmission_format = FORMAT_HEVC;
                                } else if (configurationData.transmissionType.equals("H264 (HW)")) {
                                    selectedType = VideoEncoder.Type.H264;
                                    transmission_format = FORMAT_H264;
                                } if (configurationData.transmissionType.equals("3GPP (HW)")) {
                                    selectedType = VideoEncoder.Type._3GPP;
                                    transmission_format = FORMAT_3GPP;
                                }

                                if (selectedType != VideoEncoder.Type.None) {

                                    if (videoEncoder == null) {
                                        videoEncoder = new VideoEncoder(selectedType);
                                        videoEncoder.opaqueData = new int[]{currentSize.width,currentSize.height,currentSize.width,transmission_format};
                                        videoEncoder.initialize( currentSize.width,currentSize.height,1000000, 30, 1 );
                                    } else if ( videoEncoder.type != selectedType ) {
                                        videoEncoder.setCallback(null);
                                        videoEncoder.release();
                                        videoEncoder = new VideoEncoder(selectedType);
                                        videoEncoder.opaqueData = new int[]{currentSize.width,currentSize.height,currentSize.width,transmission_format};
                                        videoEncoder.initialize( currentSize.width,currentSize.height,1000000, 30, 1 );
                                    }

                                    if (!videoEncoder.isCallbackSet()) {
                                        videoEncoder.setCallback(new VideoEncoder.Callback() {
                                            @Override
                                            public void onVideoEncoderData(byte[] data, Object opaque) {
                                                int[] params = (int[])opaque;

                                                byteBuffer.position(0);

                                                byteBuffer.putInt(data.length);
                                                byteBuffer.putInt(params[0]);//width
                                                byteBuffer.putInt(params[1]);//height
                                                byteBuffer.putInt(params[3]);// transmission type

                                                byteBuffer.put(data, 0, data.length);

                                                streamTCPServer.send(byteBuffer.array(), byteBuffer.arrayOffset(), byteBuffer.position());
                                            }
                                        });
                                    }

                                    if (streamTCPServer.hasNewConnection()) {
                                        videoEncoder.sendAllCSD();
                                    }

                                    ByteBuffer cameraData = ByteBuffer.wrap(objectBuffer.getArray());
                                    cameraData.position(objectBuffer.getSize());
                                    videoEncoder.encode420P( cameraData );

                                } else if (configurationData.transmissionType.equals("YUV420 (Zlib)")){

                                    // Compress the bytes
                                    Deflater compresser = new Deflater(Deflater.BEST_SPEED);
                                    compresser.setInput(objectBuffer.getArray(),0,objectBuffer.getSize());
                                    compresser.finish();
                                    int compressedDataLength = compresser.deflate(auxCompressBuffer);
                                    compresser.end();

                                    byteBuffer.position(0);// reset write position

                                    byteBuffer.putInt( compressedDataLength );
                                    byteBuffer.putInt( currentSize.width );
                                    byteBuffer.putInt( currentSize.height );
                                    byteBuffer.putInt(FORMAT_YUV420P_ZLIB);

                                    byteBuffer.put(auxCompressBuffer, 0, compressedDataLength);

                                    streamTCPServer.send(byteBuffer.array(), byteBuffer.arrayOffset(), byteBuffer.position());

                                } else if (configurationData.transmissionType.equals("YUV420 (RAW)")){

                                    byteBuffer.position(0);// reset write position

                                    byteBuffer.putInt( size_yuv420 );
                                    byteBuffer.putInt( currentSize.width );
                                    byteBuffer.putInt( currentSize.height );
                                    byteBuffer.putInt(FORMAT_YUV420P);

                                    byteBuffer.put(objectBuffer.getArray(), 0, objectBuffer.getSize());

                                    streamTCPServer.send(byteBuffer.array(), byteBuffer.arrayOffset(), byteBuffer.position());
                                }
                            }
                        }
                        //((GLRendererYUV2RGB)mGLView.getRenderer()).postYV12( data, size.width, size.height );

                        // return the objectBuffer to the camera pool...
                        cameraReference.bufferPool.release(objectBuffer);
                    }
                }
            );
        }

        can_exit = true;
    }

    @Override
    protected void onPause() {
        super.onPause();

        Log.d(TAG,"onPause");

        while (!can_exit) {
            try {
                Thread.sleep(100);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }

        CameraHelper.stopPreview();
        CameraHelper.CloseCamera();

        if (streamDiscovering != null){
            streamDiscovering.close();
            streamDiscovering = null;
        }

        if (streamTCPServer != null){
            streamTCPServer.close();
            streamTCPServer = null;
        }

        if (videoEncoder != null) {
            videoEncoder.setCallback(null);
            videoEncoder.release();
            videoEncoder = null;
        }

        //multicastLock.release();
    }


    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
    }

    private void toggle() {
        if (mVisible) {
            hide();
        } else {
            show();
        }
    }

    private void hide() {
        // Hide UI first
        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.hide();
        }
        //mControlsView.setVisibility(View.GONE);
        mVisible = false;

        // Schedule a runnable to remove the status and navigation bar after a delay
        mHideHandler.removeCallbacks(mShowPart2Runnable);
        mHideHandler.postDelayed(mHidePart2Runnable, UI_ANIMATION_DELAY);
    }

    @SuppressLint("InlinedApi")
    private void show() {
        // Show the system bar
        mGLView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION);
        mVisible = true;

        // Schedule a runnable to display UI elements after a delay
        mHideHandler.removeCallbacks(mHidePart2Runnable);
        mHideHandler.postDelayed(mShowPart2Runnable, UI_ANIMATION_DELAY);
    }

    /**
     * Schedules a call to hide() in [delay] milliseconds, canceling any
     * previously scheduled calls.
     */
    private void delayedHide(int delayMillis) {
        mHideHandler.removeCallbacks(mHideRunnable);
        mHideHandler.postDelayed(mHideRunnable, delayMillis);
    }
}
