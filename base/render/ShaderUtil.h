/**
 * My simple shader util source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __SHADER_UTIL_H_
#define __SHADER_UTIL_H_

#include <GLES3/gl3.h>

namespace ShaderUtil
{

void checkGlError(const char* label);

GLuint loadShader(GLenum shader_type, const char* source);

GLuint createProgram(const char* vertex, const char* fragment);

} // namespace ShaderUtil

#endif /* __SHADER_UTIL_H_ */
