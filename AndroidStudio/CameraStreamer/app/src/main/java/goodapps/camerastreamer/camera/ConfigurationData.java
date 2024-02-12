package goodapps.camerastreamer.camera;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ObjectInput;
import java.io.ObjectInputStream;
import java.io.ObjectOutput;
import java.io.ObjectOutputStream;
import java.io.Serializable;

/**
 * Created by alessandro on 19/05/2017.
 */

public class ConfigurationData implements Serializable{

    public String deviceName;
    public String transmissionType;
    public boolean useFlashLight;

    public int width;
    public int height;

    public int fps_min;
    public int fps_max;

    public int bitrate;

    public boolean use_touch_to_focus;

    public byte[] toByteArray() {
        ByteArrayOutputStream bos = new ByteArrayOutputStream();
        ObjectOutput out = null;
        try {
            out = new ObjectOutputStream(bos);
            out.writeObject(this);
            out.flush();
            //byte[] yourBytes = bos.toByteArray();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            try {
                bos.close();
            } catch (IOException ex) {
                // ignore close exception
            }
        }
        return bos.toByteArray();
    }

    public static ConfigurationData createFromByteArray(byte[] bytes) {
        Object o = null;
        ByteArrayInputStream bis = new ByteArrayInputStream(bytes);
        ObjectInput in = null;
        try {
            in = new ObjectInputStream(bis);
            o = in.readObject();
        } catch (ClassNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            try {
                if (in != null)
                    in.close();
            } catch (IOException ex) {
                // ignore close exception
            }
        }
        return (ConfigurationData)o;
    }

}
