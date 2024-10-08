/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "RawNV12Renderer.h"

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
    "uniform sampler2D u_TextureY;\n"
    "uniform sampler2D u_TextureUV;\n"
    "uniform float u_Alpha;\n"
    "uniform float u_ColorFactor;\n" // 0.0 = original, 1.0 = graysacle
    "out vec4 fragColor;\n"
    "void main()\n"
    "{\n"
    "    vec3 yuv;\n"
    "    yuv.x = texture(u_TextureY, v_TexCoords).r;\n"        // Y
    "    yuv.y = texture(u_TextureUV, v_TexCoords).r - 0.5;\n" // U
    "    yuv.z = texture(u_TextureUV, v_TexCoords).a - 0.5;\n"  // V
    "    vec3 rgb = mat3(   1.0,   1.0,  1.0,\n"
    "                       0.0,-0.344,1.770,\n"
    "                     1.403,-0.714,  0.0) * yuv;\n"
    "    vec4 pixel = vec4(rgb, 1);\n"
    "    float gray = yuv.x;\n"
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

RawNV12Renderer::RawNV12Renderer(float alpha, ColorMode_e eMode)
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

    mUniformTextureY    = glGetUniformLocation(mProgram, "u_TextureY");
    mUniformTextureUV   = glGetUniformLocation(mProgram, "u_TextureUV");

    glGenTextures(1, &mTextureIdY);
    glBindTexture(GL_TEXTURE_2D, mTextureIdY);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenTextures(1, &mTextureIdUV);
    glBindTexture(GL_TEXTURE_2D, mTextureIdUV);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    setMVP(DEFAULT_MVP);
    glBindTexture(GL_TEXTURE_2D, 0);
}

RawNV12Renderer::~RawNV12Renderer()
{
    if(mTextureIdY != (GLuint)-1)
        glDeleteTextures(1, &mTextureIdY);

    if(mTextureIdUV != (GLuint)-1)

    glDeleteProgram(mProgram);
    // TODO. IMPLEMENTS HERE
}

void RawNV12Renderer::setMVP(float* mvp)
{
    for (int ii = 0; ii < 16; ii++)
        mMVP[ii] = mvp[ii];
}

void RawNV12Renderer::setAlpha(float alpha)
{
    mAlpha = alpha;
}

void RawNV12Renderer::setColorMode(ColorMode_e eMode)
{
    mColorMode = eMode;
}

void RawNV12Renderer::onDraw(RawImageFrame* image)
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

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mTextureIdY);
    glUniform1i(mUniformTextureY, 2);

    if (image != NULL)
    {
        uint8_t* ptrY = image->mRawData;
        int height = image->mHeight;
#if 1 /* W/A code for old TX */
        if (height > 1080 && height < 1090)
            height = 1080;
#endif

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, image->mWidth, height, 0, GL_RED, GL_UNSIGNED_BYTE, ptrY);
    }

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mTextureIdUV);
    glUniform1i(mUniformTextureUV, 3);

    if (image != NULL)
    {
        int height = image->mHeight;
#if 1 /* W/A code for old TX */
        if (height > 1080 && height < 1090)
            height = 1080;
#endif
        uint8_t* ptrUV = image->mRawData + (image->mWidth * image->mHeight);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, image->mWidth / 2, height / 2, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, ptrUV);
    }

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(mAttribPos);
    glDisableVertexAttribArray(mAttribTex);

    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}
