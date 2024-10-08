/**
 * My simple shader util source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "ShaderUtil.h"

#include <vector>
#include "Log.h"

namespace ShaderUtil
{

void checkGlError(const char* label)
{
    int gl_error = glGetError();
    if (gl_error != GL_NO_ERROR)
    {
        LOGE("GL error %s: %d", label, gl_error);
    }
}

GLuint loadShader(GLenum shader_type, const char* source)
{
    GLuint shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    checkGlError("glCompileShader");
    GLint result = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE)
    {
        int log_length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
        if (log_length == 0)
            return 0;

        std::vector<char> log_string(log_length);
        glGetShaderInfoLog(shader, log_length, nullptr, log_string.data());
        LOGE("Could not compile shader of type 0x%04x: %s", shader_type, log_string.data());

        shader = 0;
    }

    return shader;
}

GLuint createProgram(const char* vertex, const char* fragment)
{
    GLuint vertex_shader = loadShader(GL_VERTEX_SHADER, vertex);
    if (vertex_shader == 0)
    {
        LOGE("Could not load vertex shader");
        return 0;
    }

    GLuint fragment_shader = loadShader(GL_FRAGMENT_SHADER, fragment);
    if (fragment_shader == 0)
    {
        LOGE("Could not load fragment shader");
        return 0;
    }

    GLuint program = glCreateProgram();

    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    checkGlError("glLinkProgram");

    GLint result = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &result);
    if (result == GL_FALSE)
    {
        int log_length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
        if (log_length == 0)
            return 0;

        std::vector<char> log_string(log_length);
        glGetShaderInfoLog(program, log_length, nullptr, log_string.data());
        LOGE("Could not compile program: %s", log_string.data());

        return 0;
    }

    glDetachShader(program, vertex_shader);
    glDetachShader(program, fragment_shader);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    checkGlError("GlCreateProgram");

    return program;
}


} // namespace ShaderUtil
