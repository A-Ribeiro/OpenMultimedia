#ifndef platform_gl__h__
#define platform_gl__h__

//#include <glad/glad/gl.h> // extensions here

//#include <OpenGL/gl3.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl3.h>
#include <InteractiveToolkit/MathCore/MathCore.h>

namespace openglWrapper {

    class PlatformGL {
    public:

#if defined(NDEBUG)

#define OPENGL_CMD(expr) expr
#define OPENGL_CMD_FL(expr, file, line) expr

#else

#define OPENGL_CMD(expr) do { \
        expr; \
        openglWrapper::PlatformGL::glCheckError(__FILE__, __LINE__, #expr); \
    } while (false)

#define OPENGL_CMD_FL(expr, file, line) do { \
        expr; \
        openglWrapper::PlatformGL::glCheckError(file, line, #expr); \
    } while (false)

#endif

        /*
#define LoadShaderProgram_macro(vertexShaderCode, fragmentShaderCode) \
    LoadShaderProgram(vertexShaderCode, fragmentShaderCode, __FILE__, __LINE__)
        */

        static void glCheckError(const char* file, unsigned int line, const char* expression);
        static void checkShaderStatus(GLuint shader, const char* file, unsigned int line, const char* aux_string);
        static void checkProgramStatus(GLuint program, const char* file, unsigned int line, const char* aux_string);

    };



}

#endif
