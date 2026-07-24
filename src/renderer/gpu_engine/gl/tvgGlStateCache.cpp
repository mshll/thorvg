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

#include <cassert>
#include "tvgArray.h"
#include "tvgGlStateCache.h"

template<typename T>
struct GlCachedValue
{
    T value = {};
    bool valid = false;

    bool matches(const T& candidate) const
    {
        return valid && value == candidate;
    }

    void set(const T& candidate)
    {
        value = candidate;
        valid = true;
    }

    void invalidate()
    {
        valid = false;
    }
};

struct GlRectState
{
    GLint x = 0;
    GLint y = 0;
    GLsizei width = 0;
    GLsizei height = 0;

    bool operator==(const GlRectState& rhs) const
    {
        return x == rhs.x && y == rhs.y && width == rhs.width && height == rhs.height;
    }
};

struct GlBlendFuncState
{
    GLenum source = 0;
    GLenum destination = 0;

    bool operator==(const GlBlendFuncState& rhs) const
    {
        return source == rhs.source && destination == rhs.destination;
    }
};

struct GlColorMaskState
{
    GLboolean red = GL_FALSE;
    GLboolean green = GL_FALSE;
    GLboolean blue = GL_FALSE;
    GLboolean alpha = GL_FALSE;

    bool operator==(const GlColorMaskState& rhs) const
    {
        return red == rhs.red && green == rhs.green && blue == rhs.blue && alpha == rhs.alpha;
    }
};

struct GlStencilFuncState
{
    GLenum function = 0;
    GLint reference = 0;
    GLuint mask = 0;

    bool operator==(const GlStencilFuncState& rhs) const
    {
        return function == rhs.function && reference == rhs.reference && mask == rhs.mask;
    }
};

struct GlStencilOpState
{
    GLenum stencilFail = 0;
    GLenum depthFail = 0;
    GLenum depthPass = 0;

    bool operator==(const GlStencilOpState& rhs) const
    {
        return stencilFail == rhs.stencilFail && depthFail == rhs.depthFail && depthPass == rhs.depthPass;
    }
};

struct GlClearColorState
{
    GLfloat red = 0.0f;
    GLfloat green = 0.0f;
    GLfloat blue = 0.0f;
    GLfloat alpha = 0.0f;

    bool operator==(const GlClearColorState& rhs) const
    {
        return red == rhs.red && green == rhs.green && blue == rhs.blue && alpha == rhs.alpha;
    }
};

struct GlVertexPointerState
{
    GLint size = 0;
    GLenum type = 0;
    GLboolean normalized = GL_FALSE;
    GLsizei stride = 0;
    size_t offset = 0;
    GLuint sourceBuffer = 0;

    bool operator==(const GlVertexPointerState& rhs) const
    {
        return size == rhs.size && type == rhs.type && normalized == rhs.normalized && stride == rhs.stride && offset == rhs.offset && sourceBuffer == rhs.sourceBuffer;
    }
};

struct GlVertexConstantState
{
    GLfloat x = 0.0f;
    GLfloat y = 0.0f;
    GLfloat z = 0.0f;
    GLfloat w = 0.0f;

    bool operator==(const GlVertexConstantState& rhs) const
    {
        return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w;
    }
};

struct GlBufferBindingState
{
    GLenum target = 0;
    GlCachedValue<GLuint> buffer;
};

struct GlTextureBindingState
{
    GLenum textureUnit = GL_TEXTURE0;
    GlCachedValue<GLuint> texture;
};

struct GlCapabilityState
{
    GLenum capability = 0;
    GlCachedValue<bool> enabled;
};

struct GlVertexAttributeState
{
    GlCachedValue<bool> enabled;
    GlCachedValue<GlVertexPointerState> pointer;
    GlCachedValue<GlVertexConstantState> constant;
    bool known = false;
    bool used = false;
};

struct GlStateCache::Impl
{
    GlCachedValue<GLuint> program;
    GlCachedValue<GLuint> vertexArray;
    GlCachedValue<GLuint> elementArrayBuffer;
    Array<GlBufferBindingState> buffers;

    GlCachedValue<GLenum> activeTextureUnit;
    Array<GlTextureBindingState> textures;
    Array<GlVertexAttributeState> vertexAttributes;

    GlCachedValue<GlRectState> viewport;
    GlCachedValue<GlRectState> scissor;
    GlCachedValue<GLuint> readFramebuffer;
    GlCachedValue<GLuint> drawFramebuffer;

    Array<GlCapabilityState> capabilities;
    GlCachedValue<GlBlendFuncState> blendFunction;
    GlCachedValue<GLenum> depthFunction;
    GlCachedValue<GLboolean> depthWriteEnabled;
    GlCachedValue<GlColorMaskState> colorWriteMask;

    GlCachedValue<GlStencilFuncState> frontStencilFunction;
    GlCachedValue<GlStencilFuncState> backStencilFunction;
    GlCachedValue<GlStencilOpState> frontStencilOperation;
    GlCachedValue<GlStencilOpState> backStencilOperation;

    GlCachedValue<GlClearColorState> clearColor;
    GlCachedValue<GLint> clearStencil;
    GlCachedValue<double> clearDepth;

    GlBufferBindingState& bufferBinding(GLenum target)
    {
        for (auto& binding : buffers) {
            if (binding.target == target) return binding;
        }
        buffers.push({target, {}});
        return buffers.last();
    }

    GlTextureBindingState& textureBinding(GLenum textureUnit)
    {
        for (auto& binding : textures) {
            if (binding.textureUnit == textureUnit) return binding;
        }
        textures.push({textureUnit, {}});
        return textures.last();
    }

    GlCapabilityState& capability(GLenum value)
    {
        for (auto& state : capabilities) {
            if (state.capability == value) return state;
        }
        capabilities.push({value, {}});
        return capabilities.last();
    }

    void setCapability(GLenum value, bool enabled)
    {
        auto& state = capability(value);
        if (state.enabled.matches(enabled)) return;
        if (enabled) {
            GL_CHECK(glEnable(value));
        } else {
            GL_CHECK(glDisable(value));
        }
        state.enabled.set(enabled);
    }

    GlVertexAttributeState& vertexAttribute(GLuint index)
    {
        while (vertexAttributes.count <= index)
            vertexAttributes.push(GlVertexAttributeState{});
        auto& attribute = vertexAttributes[index];
        attribute.known = true;
        return attribute;
    }

    void invalidateVertexArrayState()
    {
        elementArrayBuffer.invalidate();
        for (auto& attribute : vertexAttributes) {
            attribute.enabled.invalidate();
            attribute.pointer.invalidate();
            attribute.used = false;
        }
    }

    void invalidateVertexConstants()
    {
        for (auto& attribute : vertexAttributes)
            attribute.constant.invalidate();
    }

    void disableVertexAttribute(GLuint index, GlVertexAttributeState& attribute)
    {
        if (!attribute.enabled.matches(false)) {
            GL_CHECK(glDisableVertexAttribArray(index));
            attribute.enabled.set(false);
        }
    }

    void enableVertexAttribute(GLuint index, GlVertexAttributeState& attribute)
    {
        if (!attribute.enabled.matches(true)) {
            GL_CHECK(glEnableVertexAttribArray(index));
            attribute.enabled.set(true);
        }
    }
};

GlStateCache::GlStateCache() : mImpl(new Impl)
{
}

GlStateCache::~GlStateCache()
{
    delete mImpl;
}

void GlStateCache::invalidate()
{
    // This forgets CPU-side assumptions only; it deliberately issues no GL
    // reset. Applications sharing the context must restore external state
    // before GlRenderer::sync().
    mImpl->program.invalidate();
    mImpl->vertexArray.invalidate();
    mImpl->invalidateVertexArrayState();
    mImpl->invalidateVertexConstants();
    for (auto& binding : mImpl->buffers)
        binding.buffer.invalidate();

    mImpl->activeTextureUnit.invalidate();
    for (auto& binding : mImpl->textures)
        binding.texture.invalidate();

    mImpl->viewport.invalidate();
    mImpl->scissor.invalidate();
    mImpl->readFramebuffer.invalidate();
    mImpl->drawFramebuffer.invalidate();

    for (auto& capability : mImpl->capabilities)
        capability.enabled.invalidate();
    mImpl->blendFunction.invalidate();
    mImpl->depthFunction.invalidate();
    mImpl->depthWriteEnabled.invalidate();
    mImpl->colorWriteMask.invalidate();

    mImpl->frontStencilFunction.invalidate();
    mImpl->backStencilFunction.invalidate();
    mImpl->frontStencilOperation.invalidate();
    mImpl->backStencilOperation.invalidate();

    mImpl->clearColor.invalidate();
    mImpl->clearStencil.invalidate();
    mImpl->clearDepth.invalidate();
}

void GlStateCache::useProgram(GLuint program)
{
    if (mImpl->program.matches(program)) return;
    GL_CHECK(glUseProgram(program));
    mImpl->program.set(program);
}

void GlStateCache::bindVertexArray(GLuint vertexArray)
{
    if (mImpl->vertexArray.matches(vertexArray)) return;
    GL_CHECK(glBindVertexArray(vertexArray));
    mImpl->vertexArray.set(vertexArray);
    mImpl->invalidateVertexArrayState();
}

void GlStateCache::bindBuffer(GLenum target, GLuint buffer)
{
    if (target == GL_ELEMENT_ARRAY_BUFFER) {
        if (mImpl->elementArrayBuffer.matches(buffer)) return;
        GL_CHECK(glBindBuffer(target, buffer));
        mImpl->elementArrayBuffer.set(buffer);
        return;
    }

    auto& binding = mImpl->bufferBinding(target);
    if (binding.buffer.matches(buffer)) return;
    GL_CHECK(glBindBuffer(target, buffer));
    binding.buffer.set(buffer);
}

void GlStateCache::bindTexture2D(GLenum textureUnit, GLuint texture)
{
    if (!mImpl->activeTextureUnit.matches(textureUnit)) {
        GL_CHECK(glActiveTexture(textureUnit));
        mImpl->activeTextureUnit.set(textureUnit);
    }

    auto& binding = mImpl->textureBinding(textureUnit);
    if (binding.texture.matches(texture)) return;
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture));
    binding.texture.set(texture);
}

void GlStateCache::beginVertexLayout()
{
    for (auto& attribute : mImpl->vertexAttributes)
        attribute.used = false;
}

void GlStateCache::setVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized,
                                          GLsizei stride, size_t offset, GLuint sourceBuffer)
{
    bindBuffer(GL_ARRAY_BUFFER, sourceBuffer);
    auto& attribute = mImpl->vertexAttribute(index);
    attribute.used = true;
    // A draw with an enabled array leaves the generic attribute value undefined.
    // Replay the constant if this attribute later transitions back to constant mode.
    attribute.constant.invalidate();
    mImpl->enableVertexAttribute(index, attribute);

    GlVertexPointerState pointer = {size, type, normalized, stride, offset, sourceBuffer};
    if (attribute.pointer.matches(pointer)) return;

    GL_CHECK(glVertexAttribPointer(index, size, type, normalized, stride, reinterpret_cast<const void*>(offset)));
    attribute.pointer.set(pointer);
}

void GlStateCache::setVertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    auto& attribute = mImpl->vertexAttribute(index);
    attribute.used = true;
    mImpl->disableVertexAttribute(index, attribute);

    GlVertexConstantState constant = {x, y, z, w};
    if (attribute.constant.matches(constant)) return;

    GL_CHECK(glVertexAttrib4f(index, x, y, z, w));
    attribute.constant.set(constant);
}

void GlStateCache::endVertexLayout()
{
    for (uint32_t index = 0; index < mImpl->vertexAttributes.count; ++index) {
        auto& attribute = mImpl->vertexAttributes[index];
        if (!attribute.known || attribute.used) continue;
        mImpl->disableVertexAttribute(index, attribute);
    }
}

void GlStateCache::viewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    GlRectState state = {x, y, width, height};
    if (mImpl->viewport.matches(state)) return;
    GL_CHECK(glViewport(x, y, width, height));
    mImpl->viewport.set(state);
}

void GlStateCache::scissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
    GlRectState state = {x, y, width, height};
    if (mImpl->scissor.matches(state)) return;
    GL_CHECK(glScissor(x, y, width, height));
    mImpl->scissor.set(state);
}

void GlStateCache::bindFramebuffer(GLenum target, GLuint framebuffer)
{
    if (target == GL_FRAMEBUFFER) {
        if (mImpl->readFramebuffer.matches(framebuffer) && mImpl->drawFramebuffer.matches(framebuffer)) return;
        GL_CHECK(glBindFramebuffer(target, framebuffer));
        mImpl->readFramebuffer.set(framebuffer);
        mImpl->drawFramebuffer.set(framebuffer);
        return;
    }

    if (target == GL_READ_FRAMEBUFFER) {
        if (mImpl->readFramebuffer.matches(framebuffer)) return;
        GL_CHECK(glBindFramebuffer(target, framebuffer));
        mImpl->readFramebuffer.set(framebuffer);
        return;
    }

    if (target == GL_DRAW_FRAMEBUFFER) {
        if (mImpl->drawFramebuffer.matches(framebuffer)) return;
        GL_CHECK(glBindFramebuffer(target, framebuffer));
        mImpl->drawFramebuffer.set(framebuffer);
        return;
    }

    GL_CHECK(glBindFramebuffer(target, framebuffer));
}

void GlStateCache::enable(GLenum capability)
{
    mImpl->setCapability(capability, true);
}

void GlStateCache::disable(GLenum capability)
{
    mImpl->setCapability(capability, false);
}

void GlStateCache::blendFunc(GLenum source, GLenum destination)
{
    GlBlendFuncState state = {source, destination};
    if (mImpl->blendFunction.matches(state)) return;
    GL_CHECK(glBlendFunc(source, destination));
    mImpl->blendFunction.set(state);
}

void GlStateCache::depthFunc(GLenum function)
{
    if (mImpl->depthFunction.matches(function)) return;
    GL_CHECK(glDepthFunc(function));
    mImpl->depthFunction.set(function);
}

void GlStateCache::depthMask(GLboolean enabled)
{
    if (mImpl->depthWriteEnabled.matches(enabled)) return;
    GL_CHECK(glDepthMask(enabled));
    mImpl->depthWriteEnabled.set(enabled);
}

void GlStateCache::colorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    GlColorMaskState state = {red, green, blue, alpha};
    if (mImpl->colorWriteMask.matches(state)) return;
    GL_CHECK(glColorMask(red, green, blue, alpha));
    mImpl->colorWriteMask.set(state);
}

void GlStateCache::stencilFunc(GLenum function, GLint reference, GLuint mask)
{
    GlStencilFuncState state = {function, reference, mask};
    if (mImpl->frontStencilFunction.matches(state) && mImpl->backStencilFunction.matches(state)) return;
    GL_CHECK(glStencilFunc(function, reference, mask));
    mImpl->frontStencilFunction.set(state);
    mImpl->backStencilFunction.set(state);
}

void GlStateCache::stencilFuncSeparate(GLenum face, GLenum function, GLint reference, GLuint mask)
{
    GlStencilFuncState state = {function, reference, mask};
    if (face == GL_FRONT) {
        if (mImpl->frontStencilFunction.matches(state)) return;
        GL_CHECK(glStencilFuncSeparate(face, function, reference, mask));
        mImpl->frontStencilFunction.set(state);
        return;
    }

    if (face == GL_BACK) {
        if (mImpl->backStencilFunction.matches(state)) return;
        GL_CHECK(glStencilFuncSeparate(face, function, reference, mask));
        mImpl->backStencilFunction.set(state);
        return;
    }

    if (face == GL_FRONT_AND_BACK) {
        if (mImpl->frontStencilFunction.matches(state) && mImpl->backStencilFunction.matches(state)) return;
        GL_CHECK(glStencilFuncSeparate(face, function, reference, mask));
        mImpl->frontStencilFunction.set(state);
        mImpl->backStencilFunction.set(state);
        return;
    }

    GL_CHECK(glStencilFuncSeparate(face, function, reference, mask));
}

void GlStateCache::stencilOp(GLenum stencilFail, GLenum depthFail, GLenum depthPass)
{
    GlStencilOpState state = {stencilFail, depthFail, depthPass};
    if (mImpl->frontStencilOperation.matches(state) && mImpl->backStencilOperation.matches(state)) return;
    GL_CHECK(glStencilOp(stencilFail, depthFail, depthPass));
    mImpl->frontStencilOperation.set(state);
    mImpl->backStencilOperation.set(state);
}

void GlStateCache::stencilOpSeparate(GLenum face, GLenum stencilFail, GLenum depthFail, GLenum depthPass)
{
    GlStencilOpState state = {stencilFail, depthFail, depthPass};
    if (face == GL_FRONT) {
        if (mImpl->frontStencilOperation.matches(state)) return;
        GL_CHECK(glStencilOpSeparate(face, stencilFail, depthFail, depthPass));
        mImpl->frontStencilOperation.set(state);
        return;
    }

    if (face == GL_BACK) {
        if (mImpl->backStencilOperation.matches(state)) return;
        GL_CHECK(glStencilOpSeparate(face, stencilFail, depthFail, depthPass));
        mImpl->backStencilOperation.set(state);
        return;
    }

    if (face == GL_FRONT_AND_BACK) {
        if (mImpl->frontStencilOperation.matches(state) && mImpl->backStencilOperation.matches(state)) return;
        GL_CHECK(glStencilOpSeparate(face, stencilFail, depthFail, depthPass));
        mImpl->frontStencilOperation.set(state);
        mImpl->backStencilOperation.set(state);
        return;
    }

    GL_CHECK(glStencilOpSeparate(face, stencilFail, depthFail, depthPass));
}

void GlStateCache::clearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    GlClearColorState state = {red, green, blue, alpha};
    if (mImpl->clearColor.matches(state)) return;
    GL_CHECK(glClearColor(red, green, blue, alpha));
    mImpl->clearColor.set(state);
}

void GlStateCache::clearStencil(GLint value)
{
    if (mImpl->clearStencil.matches(value)) return;
    GL_CHECK(glClearStencil(value));
    mImpl->clearStencil.set(value);
}

void GlStateCache::clearDepth(double value)
{
    if (mImpl->clearDepth.matches(value)) return;
#if defined(__EMSCRIPTEN__)
    GL_CHECK(glClearDepthf(static_cast<GLfloat>(value)));
#elif defined(THORVG_GL_TARGET_GLES)
    GL_CHECK(glClearDepthf(static_cast<GLfloat>(value)));
#else
    GL_CHECK(glClearDepth(value));
#endif
    mImpl->clearDepth.set(value);
}
