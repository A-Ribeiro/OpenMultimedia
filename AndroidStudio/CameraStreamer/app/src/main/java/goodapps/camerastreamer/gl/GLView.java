package goodapps.camerastreamer.gl;

import android.content.Context;
import android.opengl.GLSurfaceView;

import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;

/**
 * Created by alessandro on 17/05/2017.
 */

public class GLView extends GLSurfaceView {

    private Renderer mRenderer = null;

    @SuppressWarnings(value = "unchecked")
    public GLView(Context context,
                  Class rendererClass
    //              GLSurfaceView.Renderer renderer
    ) {
        super(context);

        // Create an OpenGL ES 2.0 context
        setEGLContextClientVersion(2);

        //mRenderer = new GLRendererYUV2RGB(this);
        try {
            Constructor constructor = rendererClass.getConstructor( GLView.class );
            mRenderer = ( Renderer )constructor.newInstance(this);
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (InstantiationException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }


        // Set the Renderer for drawing on the GLSurfaceView
        setRenderer(mRenderer);

        // need to call requestRender to refresh this view
        setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);



    }

    public Renderer getRenderer() {

        return mRenderer;

    }

}
