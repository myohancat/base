/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "RawRGBARenderer.h"

#include "ShaderUtil.h"
#include "RenderService.h"
#include <vector>
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
    "uniform float u_ColorFactor;\n" // 0.0 = original, 1.0 = graysacle
    "out vec4 fragColor;\n"
    "void main()\n"
    "{\n"
    "    vec4 pixel = texture(u_Texture, v_TexCoords);\n"
    "    float gray  = 0.21 * pixel.r + 0.72 * pixel.g + 0.07 * pixel.b;\n"
    "    fragColor = vec4(pixel.rgb * (1.0-u_ColorFactor) + (gray*u_ColorFactor), (pixel.a*u_Alpha));\n"
    "}";

static float VERTICES[] =
{
    -1.0f, -1.0f, 0.0f,
    +1.0f, -1.0f, 0.0f,
    -1.0f, +1.0f, 0.0f,
    +1.0f, +1.0f, 0.0f
};

static float TEX_COORDS[] =
{
    0.0f, 0.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f
};

static float DEFAULT_MVP[] =
{
    1.0f,  0.0f,  0.0f,  0.0f,
    0.0f,  1.0f,  0.0f,  0.0f,
    0.0f,  0.0f,  1.0f,  0.0f,
    0.0f,  0.0f,  0.0f,  1.0f,
};

RawRGBARenderer::RawRGBARenderer(float alpha, ColorMode_e eMode)
               : mX(0), mY(0), mWidth(0), mHeight(0),
                 mAlpha(alpha),
                 mColorMode(eMode)
{
    mLock.unlock();

    mProgram = ShaderUtil::createProgram(VERTEX_SHADER_SRC, FRAGMENT_SHADER_SRC);

    mAttribPos          = glGetAttribLocation(mProgram, "a_Position");
    mAttribTex          = glGetAttribLocation(mProgram, "a_TexCoords");
    mUniformAlpha       = glGetUniformLocation(mProgram, "u_Alpha");
    mUniformColorFactor = glGetUniformLocation(mProgram, "u_ColorFactor");
    mUniformMVP         = glGetUniformLocation(mProgram, "u_MVPMatrix");
    mUniformTexture     = glGetUniformLocation(mProgram, "u_Texture");

    glGenTextures(1, &mTexture);
    glBindTexture(GL_TEXTURE_2D, mTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    setMVP(DEFAULT_MVP);
}

RawRGBARenderer::~RawRGBARenderer()
{
    if(mTexture != (GLuint)-1)
        glDeleteTextures(1, &mTexture);

    glDeleteProgram(mProgram);
    // TODO. IMPLEMENTS HERE
}

void RawRGBARenderer::setMVP(float* mvp)
{
    for (int ii = 0; ii < 16; ii++)
        mMVP[ii] = mvp[ii];
}

void RawRGBARenderer::setAlpha(float alpha)
{
    mAlpha = alpha;
}

void RawRGBARenderer::setColorMode(ColorMode_e eMode)
{
    mColorMode = eMode;
}

void RawRGBARenderer::onDraw(RawImageFrame* image)
{
    Lock lock(mLock);

    glViewport(mX, mY, mWidth, mHeight);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDisable(GL_DEPTH_TEST);
    glUseProgram(mProgram);

    glVertexAttribPointer(mAttribPos, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), VERTICES);
    glVertexAttribPointer(mAttribTex, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), TEX_COORDS);

    glEnableVertexAttribArray(mAttribPos);
    glEnableVertexAttribArray(mAttribTex);

    glUniform1f(mUniformAlpha, mAlpha);
    switch (mColorMode)
    {
        case COLOR_MODE_RGBA:
            glUniform1f(mUniformColorFactor, 0.0f);
            break;
        case COLOR_MODE_GRAYSCALE:
            glUniform1f(mUniformColorFactor, 1.0f);
            break;
        default:
            LOGW("Invalid Color Mode (%d): no change.", mColorMode);
            break;
    }
    glUniformMatrix4fv(mUniformMVP, 1, GL_FALSE, mMVP);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTexture);

    if (image != NULL)
    {
        if (image->mFmt == PIXEL_FORMAT_RGBA)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->mWidth, image->mHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->mRawData);
        else if (image->mFmt == PIXEL_FORMAT_RGB)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image->mWidth, image->mHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, image->mRawData);
        else
            LOGE("Invalid pixel format. drop it.");
    }

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(mAttribPos);
    glDisableVertexAttribArray(mAttribTex);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}
