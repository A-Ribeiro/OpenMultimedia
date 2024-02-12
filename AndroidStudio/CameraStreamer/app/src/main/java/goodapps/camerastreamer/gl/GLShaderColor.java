package goodapps.camerastreamer.gl;

import android.opengl.GLES20;

/**
 * Created by alessandro on 22/05/2017.
 */

public class GLShaderColor extends GLShader {

    private final String vertexShaderCode =
            "attribute vec4 vPosition;" +
            "attribute vec2 vUV;" +
            "uniform vec2 vScale;" +
            "uniform vec2 vSecondScale;" +
            "uniform vec2 vSecondPoint;" +
            "varying vec2 uv;" +
            "void main() {" +
            "  uv = vUV;" +
            "  vec4 p = vPosition;" +
            "  p.xy *= vScale;" +
            "  p.xy *= vSecondScale;" +
            "  p.xy += vSecondPoint;" +
            "  gl_Position = p;" +
            "}";

    private final String fragmentShaderCode =
            "precision mediump float;" +
            "varying vec2 uv;" +
            "uniform vec4 uColor;"+
            "void main() {" +
            "  vec4 result = uColor;"+
            "  gl_FragColor = result;" +
            "}";

    public int mScaleUniform;
    public int mSecondScaleUniform;
    public int mSecondPointUniform;
    public int mColorUniform;

    public float scaleX;
    public float scaleY;

    public float secondScaleX;
    public float secondScaleY;

    public GLShaderColor() {
        LoadShaderProgram( vertexShaderCode, fragmentShaderCode );

        mScaleUniform = GLES20.glGetUniformLocation(mProgram, "vScale");
        mSecondScaleUniform = GLES20.glGetUniformLocation(mProgram, "vSecondScale");
        mColorUniform = GLES20.glGetUniformLocation(mProgram, "uColor");

        mSecondPointUniform = GLES20.glGetUniformLocation(mProgram, "vSecondPoint");
    }

    public void setScale(float x, float y) {
        scaleX = x;
        scaleY = y;
        GLES20.glUniform2f(mScaleUniform,x,y);
    }
    public void setSecondScale(float x, float y) {
        secondScaleX = x;
        secondScaleY = y;
        GLES20.glUniform2f(mSecondScaleUniform,x,y);
    }

    public void setSecondPoint(float x, float y) {
        GLES20.glUniform2f(mSecondPointUniform,x,y);
    }

    public void setColor(float r, float g, float b, float a) {
        GLES20.glUniform4f(mColorUniform,r, g, b, a);
    }
}
