package goodapps.camerastreamer;

import android.Manifest;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
//import android.hardware.Camera;
//import android.support.v7.app.AppCompatActivity;
import android.content.pm.PackageManager;
import android.net.ConnectivityManager;
import android.net.LinkProperties;
import android.net.Network;
import android.os.Build;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.Spinner;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;

import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import goodapps.camerastreamer.camera.CameraHelper;
import goodapps.camerastreamer.camera.CameraReference;
import goodapps.camerastreamer.camera.ConfigurationData;
import goodapps.camerastreamer.codec.HardwareEncoderHelper;
import network.NetworkHelper;

public class StreamingConfigurationActivity extends AppCompatActivity implements Runnable {

    StreamingConfigurationActivity thiz = this;

    List<CameraReference.Size> resolutionList;
    ArrayAdapter<CameraReference.Size> resolutionAdapter;

    List<String> transmission_type_list;
    ArrayAdapter<String> transmission_type_list_adapter;

    List<String> bitrate_list;
    ArrayAdapter<String> bitrate_list_adapter;

    List<int[]> fpsList;
    ArrayAdapter<int[]> fpsAdapter;

    NetworkCallbackAdapter networkCallbackAdapter = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_streaming_configuration);

        CameraHelper.onCreateResetGLView();

        //
        // CameraHelper Resolution List
        //
        resolutionList = new ArrayList<CameraReference.Size>();
        resolutionAdapter = new ArrayAdapter<CameraReference.Size>(this, R.layout.list_element, R.id.element_text, resolutionList){
            @Override
            public View getView(int position, View convertView, ViewGroup parent){
                View row;
                if (null == convertView) {
                    LayoutInflater mInflater = (LayoutInflater) thiz.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
                    row = mInflater.inflate(R.layout.list_element, null);
                } else
                    row = convertView;
                TextView textView = (TextView)row.findViewById(R.id.element_text);
                CameraReference.Size size = getItem(position);
                textView.setText( String.format("%dx%d", size.width, size.height ) );
                return row;
            }
            @Override
            public View getDropDownView(int position, View convertView, ViewGroup parent) {
                return getView(position,convertView,parent);
            }
        };
        resolutionAdapter.setDropDownViewResource(R.layout.list_element);

        Spinner spinner_camera_resolution = (Spinner)findViewById( R.id.spinner_camera_resolution );
        spinner_camera_resolution.setAdapter( resolutionAdapter );
        spinner_camera_resolution.setSelection(0);


        fpsList = new ArrayList<int[]>();
        fpsAdapter = new ArrayAdapter<int[]>(this, R.layout.list_element, R.id.element_text, fpsList){
            @Override
            public View getView(int position, View convertView, ViewGroup parent){
                View row;
                if (null == convertView) {
                    LayoutInflater mInflater = (LayoutInflater) thiz.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
                    row = mInflater.inflate(R.layout.list_element, null);
                } else
                    row = convertView;
                TextView textView = (TextView)row.findViewById(R.id.element_text);
                int[] fps = getItem(position);
                textView.setText( String.format("%.2f to %.2f", (float)fps[0]/1000.0f, (float)fps[1]/1000.0f ) );
                return row;
            }
            @Override
            public View getDropDownView(int position, View convertView, ViewGroup parent) {
                return getView(position,convertView,parent);
            }
        };
        fpsAdapter.setDropDownViewResource(R.layout.list_element);

        Spinner spinner_camera_fps = (Spinner)findViewById( R.id.spinner_camera_fps );
        spinner_camera_fps.setAdapter( fpsAdapter );
        spinner_camera_fps.setSelection(0);

        //
        // Transmission Types
        //
        transmission_type_list = new ArrayList<String>();

        // if have codec available

        if (HardwareEncoderHelper.getVideoEncoder(HardwareEncoderHelper.VIDEO_MIME_TYPE_HEVC) != null)
            transmission_type_list.add("HEVC (HW)");

        if (HardwareEncoderHelper.getVideoEncoder(HardwareEncoderHelper.VIDEO_MIME_TYPE_H264) != null)
            transmission_type_list.add("H264 (HW)");

        if (HardwareEncoderHelper.getVideoEncoder(HardwareEncoderHelper.VIDEO_MIME_TYPE_3GPP) != null)
            transmission_type_list.add("3GPP (HW)");

        transmission_type_list.add("YUV420 (Zlib)");
        transmission_type_list.add("YUV420 (RAW)");
        transmission_type_list_adapter = new ArrayAdapter<String>(this, R.layout.list_element, R.id.element_text, transmission_type_list);
        transmission_type_list_adapter.setDropDownViewResource(R.layout.list_element);

        Spinner transmission_type = (Spinner)findViewById(R.id.spinner_transmission_type);
        transmission_type.setAdapter( transmission_type_list_adapter );
        transmission_type.setSelection( 0 );

        //
        // Bitrate
        //
        bitrate_list = new ArrayList<String>();
        bitrate_list.add("1500000");
        bitrate_list.add("2500000");
        bitrate_list.add("3500000");
        bitrate_list.add("4500000");
        bitrate_list.add("5500000");

        bitrate_list_adapter = new ArrayAdapter<String>(this, R.layout.list_element, R.id.element_text, bitrate_list);
        bitrate_list_adapter.setDropDownViewResource(R.layout.list_element);

        Spinner bitrate_type = (Spinner)findViewById(R.id.spinner_codec_bitrate);
        bitrate_type.setAdapter( bitrate_list_adapter );
        bitrate_type.setSelection( 0 );




        //
        // Start Button
        //
        Button startButton = (Button)findViewById(R.id.button_start);

        startButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                action_start();
            }
        });

    }

    public void SetIPList() {
        //
        // IPList
        //
        String aux = "--- IP List ---";

        for(String ip: NetworkHelper.lan_wlan_ips( NetworkHelper.InterfaceType.IPv4 )){
            aux += "\n" + ip;
        }

        TextView ipList = (TextView)findViewById( R.id.ip_list );

        ipList.setText( aux );

    }

    private static String getDeviceName() {
        //String device = Build.DEVICE;
        //String model = Build.MODEL;
        String manufacturer = Build.MANUFACTURER;
        String model = Build.MODEL;
        if (model.startsWith(manufacturer)) {
            return capitalize(model);
        }
        return capitalize(manufacturer) + " " + model;
    }

    private static String capitalize(String str) {
        if (TextUtils.isEmpty(str)) {
            return str;
        }
        char[] arr = str.toCharArray();
        boolean capitalizeNext = true;
        StringBuilder phrase = new StringBuilder();
        for (char c : arr) {
            if (capitalizeNext && Character.isLetter(c)) {
                phrase.append(Character.toUpperCase(c));
                capitalizeNext = false;
                continue;
            } else if (Character.isWhitespace(c)) {
                capitalizeNext = true;
            }
            phrase.append(c);
        }
        return phrase.toString();
    }

    @Override
    public void run() {

        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED)
            return;

        //get camera resolutions
        List<CameraReference.Size> resolutions = new ArrayList<CameraReference.Size>();
        List<int[]> fps = new ArrayList<int[]>();

        int cameraID = CameraReference.getCameraFacingBack( this );
        int captureImageFormat = CameraReference.chooseCompatibleCameraFormat(this, cameraID);

        CameraHelper.getCameraResolutionAndFPS(this, cameraID, resolutions, fps, captureImageFormat);

        int res_w = 1920;
        int res_h = 1080;
        boolean res_set = false;

        resolutionList.clear();
        for(CameraReference.Size res : resolutions) {
            //pick a start value for the configuration
            if (!res_set && res.width <= res_w && res.height <= res_h) {
                res_set = true;
                res_w = res.width;
                res_h = res.height;
            }
            resolutionList.add(res);
        }
        resolutionAdapter.notifyDataSetChanged();

        int[] initial_fps = new int[]{15000,15000};
        fpsList.clear();
        for(int[] res:fps)
            fpsList.add(res);
        if (fpsList.size()>0)
            initial_fps = fpsList.get(0);
        fpsAdapter.notifyDataSetChanged();

        TextView device_name = (TextView)findViewById(R.id.editText_device_name);
        Spinner spinner_transmission_type = (Spinner)findViewById(R.id.spinner_transmission_type);
        CheckBox use_flashlight = (CheckBox)findViewById(R.id.checkBox_use_flashlight);
        CheckBox use_touch_to_focus = (CheckBox)findViewById(R.id.checkBox_use_touch_to_focus);
        Spinner spinner_camera_resolution = (Spinner)findViewById( R.id.spinner_camera_resolution );
        Spinner spinner_camera_fps = (Spinner)findViewById( R.id.spinner_camera_fps );
        Spinner spinner_bitrate = (Spinner)findViewById( R.id.spinner_codec_bitrate );


        //load preferences
        SharedPreferences sharedPref = this.getPreferences(Context.MODE_PRIVATE);
        String device_name_pref = sharedPref.getString("device name", getDeviceName() ); // default device name
        String transmission_type_pref = sharedPref.getString("transmission type", transmission_type_list.get(0).toString()); // default transmission type
        int channel_w_pref = sharedPref.getInt("camera width", res_w );
        int channel_h_pref = sharedPref.getInt("camera height", res_h );
        int fps_min = sharedPref.getInt("camera fps min", initial_fps[0] );
        int fps_max = sharedPref.getInt("camera fps max", initial_fps[1] );
        String bitrate = sharedPref.getString("codec bitrate", bitrate_list.get(1).toString() );
        boolean use_flashlight_pref = sharedPref.getBoolean("use flashlight", false );
        boolean use_TouchToFocus_pref = sharedPref.getBoolean("use touch to focus", false );

        device_name.setText(device_name_pref);
        use_flashlight.setChecked( use_flashlight_pref );
        use_touch_to_focus.setChecked(use_TouchToFocus_pref);

        //transmission type
        for(int i=0;i<transmission_type_list.size();i++) {
            String transmissionTypeStr = transmission_type_list.get(i);
            if (transmissionTypeStr.equals(transmission_type_pref)){
                spinner_transmission_type.setSelection(i);
                break;
            }
        }

        //resolution
        for(int i=0;i<resolutionList.size();i++) {
            CameraReference.Size size = resolutionList.get(i);
            if (size.width == channel_w_pref && size.height == channel_h_pref){
                spinner_camera_resolution.setSelection(i);
                break;
            }
        }
        //fps
        for(int i=0;i<fpsList.size();i++) {
            int[] element = fpsList.get(i);
            if (element[0] == fps_min && element[1] == fps_max){
                spinner_camera_fps.setSelection(i);
                break;
            }
        }
        //bitrate
        for(int i=0;i<bitrate_list.size();i++) {
            String element = bitrate_list.get(i);
            if (element.equals(bitrate)){
                spinner_bitrate.setSelection(i);
                break;
            }
        }

    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           String permissions[], int[] grantResults) {
        switch (requestCode) {
            case 101:
                if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    this.runOnUiThread(this);
                } else {
                }
                break;
        }
    }

    @Override
    protected void onResume(){
        super.onResume();

        SetIPList();

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {

            if (networkCallbackAdapter == null)
                networkCallbackAdapter = new NetworkCallbackAdapter(this);

            ConnectivityManager manager = (ConnectivityManager) this.getSystemService(Context.CONNECTIVITY_SERVICE);
            manager.registerDefaultNetworkCallback (networkCallbackAdapter);
        }

        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED){

            ActivityCompat.requestPermissions(this,
                    new String[]{
                            Manifest.permission.CAMERA
                    }, 101);

        } else {
            this.runOnUiThread(this);
        }
    }

    @Override
    protected void onPause() {
        super.onPause();

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            if (networkCallbackAdapter != null) {
                ConnectivityManager manager = (ConnectivityManager) this.getSystemService(Context.CONNECTIVITY_SERVICE);
                manager.unregisterNetworkCallback(networkCallbackAdapter);
            }
        }

        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED)
            return;

        TextView device_name = (TextView)findViewById(R.id.editText_device_name);
        Spinner spinner_transmission_type = (Spinner)findViewById(R.id.spinner_transmission_type);
        CheckBox use_flashlight = (CheckBox)findViewById(R.id.checkBox_use_flashlight);
        CheckBox use_touch_to_focus = (CheckBox)findViewById(R.id.checkBox_use_touch_to_focus);
        Spinner spinner_camera_resolution = (Spinner)findViewById( R.id.spinner_camera_resolution );
        Spinner spinner_camera_fps = (Spinner)findViewById( R.id.spinner_camera_fps );
        Spinner spinner_bitrate = (Spinner)findViewById( R.id.spinner_codec_bitrate );

        //save preferences
        SharedPreferences sharedPref = this.getPreferences(Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = sharedPref.edit();
        editor.putString("device name", device_name.getText().toString());
        editor.putString("transmission type", spinner_transmission_type.getSelectedItem().toString());
        editor.putBoolean("use flashlight", use_flashlight.isChecked() );
        editor.putBoolean("use touch to focus",use_touch_to_focus.isChecked());

        CameraReference.Size cameraSize = (CameraReference.Size)spinner_camera_resolution.getSelectedItem();
        if (cameraSize != null) {
            editor.putInt("camera width", cameraSize.width );
            editor.putInt("camera height", cameraSize.height );
        }

        int[] fpsSelected = (int[])spinner_camera_fps.getSelectedItem();
        if (fpsSelected != null)
        {
            editor.putInt("camera fps min", fpsSelected[0] );
            editor.putInt("camera fps max", fpsSelected[1] );
        }

        editor.putString("codec bitrate", spinner_bitrate.getSelectedItem().toString());

        editor.commit();

    }

    void action_start()
    {
        TextView device_name = (TextView)findViewById(R.id.editText_device_name);
        Spinner transmission_type = (Spinner)findViewById(R.id.spinner_transmission_type);
        CheckBox use_flashlight = (CheckBox)findViewById(R.id.checkBox_use_flashlight);
        CheckBox use_touch_to_focus = (CheckBox)findViewById(R.id.checkBox_use_touch_to_focus);

        Spinner spinner_camera_resolution = (Spinner)findViewById( R.id.spinner_camera_resolution );
        Spinner spinner_camera_fps = (Spinner)findViewById( R.id.spinner_camera_fps );
        Spinner spinner_bitrate = (Spinner)findViewById( R.id.spinner_codec_bitrate );

        CameraReference.Size cameraSize = (CameraReference.Size)spinner_camera_resolution.getSelectedItem();
        int[] fpsSelected = (int[])spinner_camera_fps.getSelectedItem();

        if (cameraSize == null || fpsSelected == null)
            return;

        //
        // prepare intent parameter
        //
        ConfigurationData configurationData = new ConfigurationData();

        configurationData.deviceName = device_name.getText().toString();
        configurationData.transmissionType = transmission_type.getSelectedItem().toString();
        configurationData.useFlashLight = use_flashlight.isChecked();
        configurationData.use_touch_to_focus = use_touch_to_focus.isChecked();
        configurationData.width = cameraSize.width;
        configurationData.height = cameraSize.height;
        configurationData.fps_min = fpsSelected[0];
        configurationData.fps_max = fpsSelected[1];
        configurationData.bitrate = Integer.parseInt(spinner_bitrate.getSelectedItem().toString());



        //
        // call activity with parameters...
        //
        Intent intent = new Intent( thiz, CameraStreamingGLViewActivity.class );
        intent.putExtra("ConfigurationData", configurationData.toByteArray());
        startActivity(intent);

    }


    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    class NetworkCallbackAdapter extends ConnectivityManager.NetworkCallback {
        public StreamingConfigurationActivity streamingConfigurationActivity;
        public NetworkCallbackAdapter( StreamingConfigurationActivity streamingConfigurationActivity){
            this.streamingConfigurationActivity = streamingConfigurationActivity;
        }

        @Override
        public void onLinkPropertiesChanged(@androidx.annotation.NonNull Network network, @androidx.annotation.NonNull LinkProperties linkProperties) {
            super.onLinkPropertiesChanged(network, linkProperties);

            streamingConfigurationActivity.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    streamingConfigurationActivity.SetIPList();
                }
            });
        }
    }


}
