package goodapps.camerastreamer.gl;

import android.opengl.GLES20;

/**
 * Created by alessandro on 17/05/2017.
 */

public class GLShaderYUV2RGB extends GLShader {

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
        "precision mediump float;" +
        "varying vec2 uv;" +
        "uniform sampler2D textureY;"+
        "uniform sampler2D textureU;"+
        "uniform sampler2D textureV;"+
        "vec3 yuv2rgb(vec3 yuv) {"+
            "yuv.x = yuv.x * 1.1640625 - 0.0710784313725490196078431372549;"+
            "yuv.yz -= 0.5;"+
            "return yuv.xxx + vec3("+
                "2.015625 * yuv.y,"+
                "-0.390625 * yuv.y - 0.8125 * yuv.z,"+
                "1.59765625 * yuv.z"+
            ");"+
        "}"+
        "void main() {" +
        "  vec3 yuv;" +
        "  yuv.x = texture2D(textureY, uv).a;" +
        "  yuv.y = texture2D(textureU, uv).a;" +
        "  yuv.z = texture2D(textureV, uv).a;" +
        "  vec4 result = vec4(0.0,0.0,0.0,1.0);"+
        "  result.rgb = yuv2rgb(yuv);" +
        "  gl_FragColor = result;" +
        "}";


    public int mScaleUniform;
    public int mTextureY;
    public int mTextureU;
    public int mTextureV;

    public GLShaderYUV2RGB() {

        LoadShaderProgram( vertexShaderCode, fragmentShaderCode );

        mScaleUniform = GLES20.glGetUniformLocation(mProgram, "vScale");
        mTextureY = GLES20.glGetUniformLocation(mProgram, "textureY");
        mTextureU = GLES20.glGetUniformLocation(mProgram, "textureU");
        mTextureV = GLES20.glGetUniformLocation(mProgram, "textureV");
    }

    public void setScale(float x, float y) {
        GLES20.glUniform2f(mScaleUniform,x,y);
    }

    public void setTextureY(int activeTexture) {
        GLES20.glUniform1i(mTextureY,activeTexture);
    }
    public void setTextureU(int activeTexture) {
        GLES20.glUniform1i(mTextureU,activeTexture);
    }
    public void setTextureV(int activeTexture) {
        GLES20.glUniform1i(mTextureV,activeTexture);
    }
}
