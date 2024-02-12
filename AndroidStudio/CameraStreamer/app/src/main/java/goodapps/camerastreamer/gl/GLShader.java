package goodapps.camerastreamer.gl;

import android.opengl.GLES20;

/**
 * Created by alessandro on 18/05/2017.
 */

public class GLShader {

    public int mProgram;
    public int mPositionAttrib;
    public int mUVAttrib;


    protected void LoadShaderProgram(String vertexShaderCode, String fragmentShaderCode) {
        mProgram = GLHelper.loadShaderStrings(vertexShaderCode,fragmentShaderCode);
        mPositionAttrib = GLES20.glGetAttribLocation(mProgram, "vPosition");
        mUVAttrib = GLES20.glGetAttribLocation(mProgram, "vUV");
    }

    public void EnableShader() {
        GLES20.glUseProgram(mProgram);
    }

    public void EnableVertexAttribArray() {
        GLES20.glEnableVertexAttribArray(mPositionAttrib);
        GLES20.glEnableVertexAttribArray(mUVAttrib);
    }

    public void DisableVertexAttribArray() {
        GLES20.glDisableVertexAttribArray(mPositionAttrib);
        GLES20.glDisableVertexAttribArray(mUVAttrib);
    }

}
