package goodapps.camerastreamer.gl;

import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.util.Log;

import java.nio.ByteBuffer;

/**
 * Created by alessandro on 17/05/2017.
 */

public class GLTexture {

    public int mTexture;
    public int width = 0;
    public int height = 0;
    int textureTarget;

    public GLTexture(boolean GL_OES_EGL_image_external) {

        int[] textures = new int[1];
        GLES20.glGenTextures(1, textures, 0);
        mTexture = textures[0];

        if (GL_OES_EGL_image_external)
            textureTarget = GLES11Ext.GL_TEXTURE_EXTERNAL_OES;
        else
            textureTarget = GLES20.GL_TEXTURE_2D;

        GLES20.glBindTexture(textureTarget, mTexture);

        // Set filtering
        GLES20.glTexParameteri(textureTarget, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
        GLES20.glTexParameteri(textureTarget, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);

        GLES20.glTexParameteri(textureTarget, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameteri(textureTarget, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);

        GLES20.glBindTexture(textureTarget, 0);
    }

    public boolean isInitialized() {
        return width > 0 && height > 0;
    }

    public void uploadBufferRGBA(byte[] buffer, int w, int h) {

        //if (buffer.length != w*h*4)
            //Log.d("ERROR"," Texture with wrong dimention and data... w: " + w +" h:"+ h);

        GLES20.glBindTexture(textureTarget, mTexture);

        if (w != width || h != height) {
            GLES20.glTexImage2D(
                    GLES20.GL_TEXTURE_2D, 0,
                    GLES20.GL_RGBA,
                    w, h, 0,
                    GLES20.GL_RGBA,
                    GLES20.GL_UNSIGNED_BYTE,
                    ByteBuffer.wrap(buffer));
            width = w;
            height = h;
        }else
            GLES20.glTexSubImage2D( GLES20.GL_TEXTURE_2D,0,
                    0,0,
                    w,h,
                    GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, ByteBuffer.wrap(buffer));

        //GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
    }

    byte[] greatestByteBufferAllocated = null;

    public void uploadBufferAlpha8(byte[] buffer, int w, int h,int stride, int bufferOffset) {
        GLES20.glBindTexture(textureTarget, mTexture);

        //set pack data alignment to 1 byte read from memory and 1 byte to write to gpu memory...
        GLES20.glPixelStorei(GLES20.GL_PACK_ALIGNMENT, 1);
        GLES20.glPixelStorei(GLES20.GL_UNPACK_ALIGNMENT, 1);

        ByteBuffer byteBuffer;// = ByteBuffer.wrap(buffer);

        if (bufferOffset == 0 && stride == w) {
            byteBuffer = ByteBuffer.wrap(buffer);
            //byteBuffer.position(bufferOffset);
        } else {
            //do extra copy of the buffer if the offset is not zero...
            if (buffer.length >= bufferOffset + stride*h) {

                if (greatestByteBufferAllocated == null)
                    greatestByteBufferAllocated = new byte[ w * h ];
                else if ( w * h > greatestByteBufferAllocated.length )
                    greatestByteBufferAllocated = new byte[ w * h ];

                byteBuffer = ByteBuffer.wrap(greatestByteBufferAllocated);
                //byteBuffer.position(0);

                for(int i=0;i<h;i++)
                    System.arraycopy( buffer, bufferOffset + stride*i, greatestByteBufferAllocated, w*i, w );
                    //byteBuffer.put(buffer, bufferOffset + stride*i, w);
                //byteBuffer.position(0);
            } else {
                Log.d("uploadBufferAlpha8","buffer access overflow...");
                return;
            }
        }

        //byteBuffer.position(bufferOffset);

        if (w != width || h != height) {
            GLES20.glTexImage2D(
                    GLES20.GL_TEXTURE_2D, 0,
                    GLES20.GL_ALPHA,
                    w, h, 0,
                    GLES20.GL_ALPHA,
                    GLES20.GL_UNSIGNED_BYTE,
                    byteBuffer);
            width = w;
            height = h;
        }else
            GLES20.glTexSubImage2D( GLES20.GL_TEXTURE_2D,0,
                    0,0, // offset
                    w,h, // dimension
                    GLES20.GL_ALPHA, GLES20.GL_UNSIGNED_BYTE, byteBuffer);

        //GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
    }


    public void active(int id){

        GLES20.glActiveTexture(GLES20.GL_TEXTURE0 + id);
        //GLES20.glEnable(GLES20.GL_TEXTURE_2D);
        GLES20.glBindTexture(textureTarget, mTexture);

    }

    public static void deactive(int id){
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0 + id);
        GLES20.glDisable(GLES20.GL_TEXTURE_2D);
    }

    public void delete()
    {
        if (mTexture != 0) {
            if (GLES20.glIsTexture(mTexture) ) {
                int[] textures = new int[]{mTexture};
                GLES20.glDeleteTextures(1, textures, 0);
            }
        }
        mTexture = 0;
    }

    @Override
    protected void finalize() throws Throwable {
        try {
            delete();
        } finally {
            super.finalize(); //To change body of generated methods, choose Tools | Templates.
        }
    }
}
