/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "EGLImageRenderer.h"

#include "ShaderUtil.h"
#include <vector>
#include "EGLHelper.h"
#include "RenderService.h"
#include "Log.h"

static char VERTEX_SHADER_SRC[] = R"(#version 300 es
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoords;
uniform mat4 u_MVPMatrix;
out vec2 v_TexCoords;
void main()
{
    gl_Position = u_MVPMatrix * vec4(a_Position.xyz, 1.0);
    v_TexCoords = a_TexCoords;
}
)";

static char FRAGMENT_SHADER_SRC[] = R"(#version 300 es
#extension GL_OES_EGL_image_external_essl3 : require
precision highp float;
in vec2 v_TexCoords;
uniform samplerExternalOES u_Texture;
uniform float u_Alpha;
uniform float u_ColorFactor; // 0.0 = original, 1.0 = graysacle
out vec4 fragColor;
void main()
{
    vec4 pixel = texture(u_Texture, v_TexCoords);
    float gray = 0.21 * pixel.r + 0.72 * pixel.g + 0.07 * pixel.b;
    fragColor  = vec4(pixel.rgb * (1.0-u_ColorFactor) + (gray*u_ColorFactor), u_Alpha);
}
)";

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

EGLImageRenderer::EGLImageRenderer(float alpha, ColorMode_e eMode)
                 : mX(0), mY(0), mWidth(0), mHeight(0),
                   mCropX(0), mCropY(0), mCropWidth(0), mCropHeight(0),
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
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, mTexture);

    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    setMVP(DEFAULT_MVP);
    memcpy(mTexCoords, TEX_COORDS, sizeof(mTexCoords));
}

EGLImageRenderer::~EGLImageRenderer()
{
    if(mTexture != (GLuint)-1)
        glDeleteTextures(1, &mTexture);

    glDeleteProgram(mProgram);
    // TODO. IMPLEMENTS HERE
}

void EGLImageRenderer::setMVP(float* mvp)
{
    for (int ii = 0; ii < 16; ii++)
        mMVP[ii] = mvp[ii];
}

void EGLImageRenderer::setCrop(int x, int y, int width, int height)
{
    mCropX = x;
    mCropY = y;
    mCropWidth = width;
    mCropHeight = height;

}

void EGLImageRenderer::applyCrop(int imgWidth, int imgHeight)
{
    if (mCropWidth <= 0 || mCropHeight <= 0)
        return;

    float normalizedX1 = (float)mCropX / imgWidth;
    float normalizedX2 = (float)(mCropX + mCropWidth) / imgWidth;
    float normalizedY1 = (float)mCropY / (float)imgHeight;
    float normalizedY2 = (float)(mCropY + mCropHeight) / imgHeight;

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

void EGLImageRenderer::setAlpha(float alpha)
{
    mAlpha = alpha;
}

void EGLImageRenderer::setColorMode(ColorMode_e eMode)
{
    mColorMode = eMode;
}

void EGLImageRenderer::draw(ImageFrame* image)
{
    Lock lock(mLock);

    if (image && image->mMemType != MEM_TYPE_EGL)
    {
        LOGW("Invalid memory type. type : %d", image->mMemType);
        return;
    }

    glViewport(mX, mY, mWidth, mHeight);
    if (image)
        applyCrop(image->mWidth, image->mHeight);

    if (mAlpha < 1)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    glDisable(GL_DEPTH_TEST);
    glUseProgram(mProgram);

    glVertexAttribPointer(mAttribPos, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), VERTICES);
    glVertexAttribPointer(mAttribTex, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), mTexCoords);

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
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, mTexture);
    glUniform1i(mUniformTexture, 0);

    if (image != NULL)
    {
        glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, image->mEglImg);
        ShaderUtil::checkGlError("glEGLImageTargetTexture2DOES");
    }

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(mAttribPos);
    glDisableVertexAttribArray(mAttribTex);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);

    if (mAlpha < 1)
        glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}
