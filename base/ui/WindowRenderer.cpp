/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "WindowRenderer.h"

#include "Window.h"
#include "ShaderUtil.h"
#include <vector>
#include <GLES/gl.h>
#include <GLES2/gl2ext.h>

#include "Log.h"

static char VERTEX_SHADER_SRC[] =
    "#version 300 es\n"
    "layout(location = 0) in vec3 a_Position;\n"
    "layout(location = 1) in vec2 a_TexCoords;\n"
    "uniform mat4 u_MVPMatrix;\n"
    "out vec2 v_TexCoords;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = u_MVPMatrix * vec4(a_Position.xyz, 1.0);\n"
    "    v_TexCoords = a_TexCoords;\n"
    "}";

static char FRAGMENT_SHADER_SRC[] =
    "#version 300 es\n"
    "precision highp float;\n"
    "in vec2 v_TexCoords;\n"
    "uniform sampler2D u_Texture;\n"
    "uniform float u_Alpha;\n"
    "out vec4 fragColor;\n"
    "void main()\n"
    "{\n"
    "    vec4 pixel = texture(u_Texture, v_TexCoords);\n"
    "    fragColor = vec4(pixel.rgb, (pixel.a*u_Alpha));\n"
    "}";

static float VERTICES[] =
{
    -1.0f, -1.0f, 0.0f,
    +1.0f, -1.0f, 0.0f,
    -1.0f, +1.0f, 0.0f,
    +1.0f, +1.0f, 0.0f
};

static float DEFAULT_MVP[] =
{
    1.0f,  0.0f,  0.0f,  0.0f,
    0.0f,  1.0f,  0.0f,  0.0f,
    0.0f,  0.0f,  1.0f,  0.0f,
    0.0f,  0.0f,  0.0f,  1.0f,
};

WindowRenderer::WindowRenderer(Window* window)
           : mWindow(window)
{
    mProgram = ShaderUtil::createProgram(VERTEX_SHADER_SRC, FRAGMENT_SHADER_SRC);

    mAttribPos      = glGetAttribLocation(mProgram, "a_Position");
    mAttribTex      = glGetAttribLocation(mProgram, "a_TexCoords");
    mUniformAlpha   = glGetUniformLocation(mProgram, "u_Alpha");
    mUniformMVP     = glGetUniformLocation(mProgram, "u_MVPMatrix");
    mUniformTexture = glGetUniformLocation(mProgram, "u_Texture");

    glGenTextures(1, &mTexture);
    glBindTexture(GL_TEXTURE_2D, mTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    setMVP(DEFAULT_MVP);
}

WindowRenderer::~WindowRenderer()
{
    if(mTexture != (GLuint)-1)
        glDeleteTextures(1, &mTexture);

    glDeleteProgram(mProgram);
}

void WindowRenderer::setMVP(float* mvp)
{
    for (int ii = 0; ii < 16; ii++)
        mMVP[ii] = mvp[ii];
}

void WindowRenderer::onDraw(void* pixels)
{
    Lock lock(mLock);

    if (mWindow == NULL)
        return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    //glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(mProgram);

    glUniform1f(mUniformAlpha, mWindow->getAlpha());

    glUniformMatrix4fv(mUniformMVP, 1, GL_FALSE, mMVP);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTexture);

    if (pixels)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mWindow->getWidth(), mWindow->getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    //glUniform1i(mUniformTexture, 0);
    setCrop(0, 0, mWindow->getWidth(), mWindow->getHeight());

    glVertexAttribPointer(mAttribPos, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), VERTICES);
    glVertexAttribPointer(mAttribTex, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), mTexCoords);

    glEnableVertexAttribArray(mAttribPos);
    glEnableVertexAttribArray(mAttribTex);

    glViewport(mWindow->getX(), mWindow->getY(), mWindow->getWidth(), mWindow->getHeight());
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(mAttribPos);
    glDisableVertexAttribArray(mAttribTex);

    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void WindowRenderer::setCrop(int x, int y, int width, int height)
{
    float normalizedX1 = (float)x / (float)mWindow->getWidth();
    float normalizedX2 = (float)(x + width) / (float)mWindow->getWidth();
    float normalizedY1 = (float)y / (float)mWindow->getHeight();
    float normalizedY2 = (float)(y + height) / (float)mWindow->getHeight();

    // 좌측 하단(0.0f, 0.0f)
    mTexCoords[0] = normalizedX1;
    mTexCoords[1] = normalizedY1;

    // 우측 하단(1.0f, 0.0f)
    mTexCoords[2] = normalizedX2;
    mTexCoords[3] = normalizedY1;

    // 좌측 상단(0.0f, 1.0f)
    mTexCoords[4] = normalizedX1;
    mTexCoords[5] = normalizedY2;

    // 우측 상단(1.0f, 1.0f)
    mTexCoords[6] = normalizedX2;
    mTexCoords[7] = normalizedY2;
}
