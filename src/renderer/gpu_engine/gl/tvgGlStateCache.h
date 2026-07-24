/*
 * Copyright (c) 2026 ThorVG project. All rights reserved.

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _TVG_GL_STATE_CACHE_H_
#define _TVG_GL_STATE_CACHE_H_

#include <cstddef>
#include "tvgGl.h"

class GlStateCache
{
public:
    GlStateCache();
    ~GlStateCache();

    GlStateCache(const GlStateCache&) = delete;
    GlStateCache& operator=(const GlStateCache&) = delete;

    void invalidate();

    void useProgram(GLuint program);
    void bindVertexArray(GLuint vertexArray);
    void bindBuffer(GLenum target, GLuint buffer);

    void bindTexture2D(GLenum textureUnit, GLuint texture);

    void beginVertexLayout();
    void setVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized,
                                GLsizei stride, size_t offset, GLuint sourceBuffer);
    void setVertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void endVertexLayout();

    void viewport(GLint x, GLint y, GLsizei width, GLsizei height);
    void scissor(GLint x, GLint y, GLsizei width, GLsizei height);
    void bindFramebuffer(GLenum target, GLuint framebuffer);

    void enable(GLenum capability);
    void disable(GLenum capability);

    void blendFunc(GLenum source, GLenum destination);
    void depthFunc(GLenum function);
    void depthMask(GLboolean enabled);
    void colorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);

    void stencilFunc(GLenum function, GLint reference, GLuint mask);
    void stencilFuncSeparate(GLenum face, GLenum function, GLint reference, GLuint mask);
    void stencilOp(GLenum stencilFail, GLenum depthFail, GLenum depthPass);
    void stencilOpSeparate(GLenum face, GLenum stencilFail, GLenum depthFail, GLenum depthPass);

    void clearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
    void clearStencil(GLint value);
    void clearDepth(double value);

private:
    struct Impl;
    Impl* mImpl;
};

#endif /* _TVG_GL_STATE_CACHE_H_ */
