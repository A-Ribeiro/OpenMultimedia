package goodapps.camerastreamer.gl;

import android.opengl.GLES20;

/**
 * Created by alessandro on 22/05/2017.
 */

public class GLShaderRGB extends GLShader {

    private final String vertexShaderCode =
            "attribute vec4 vPosition;" +
            "attribute vec2 vUV;" +
            "uniform vec2 vScale;" +
            "varying vec2 uv;" +
            "void main() {" +
            "  uv = vUV;" +
            "  vec4 p = vPosition;" +
            "  p.xy *= vScale;" +
            "  gl_Position = p;" +
            "}";

    private final String fragmentShaderCode =
            "#extension GL_OES_EGL_image_external : require \n" +
            "precision mediump float;" +
            "varying vec2 uv;" +
            "uniform samplerExternalOES textureRGB;"+
            "void main() {" +
            "  vec4 result = vec4(0.0,0.0,0.0,1.0);"+
            "  result.rgb = texture2D(textureRGB, uv).rgb;" +
            //"  result.r = 1.0;" +
            "  gl_FragColor = result;" +
            "}";

    public int mScaleUniform;
    public int textureRGB;

    public GLShaderRGB() {
        LoadShaderProgram( vertexShaderCode, fragmentShaderCode );

        mScaleUniform = GLES20.glGetUniformLocation(mProgram, "vScale");
        textureRGB = GLES20.glGetUniformLocation(mProgram, "textureRGB");
    }

    public void setScale(float x, float y) {
        GLES20.glUniform2f(mScaleUniform,x,y);
    }

    public void setTextureRGB(int activeTexture) {
        GLES20.glUniform1i(textureRGB,activeTexture);
    }
}
