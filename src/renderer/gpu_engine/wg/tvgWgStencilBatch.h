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

#ifndef _TVG_WG_STENCIL_BATCH_H_
#define _TVG_WG_STENCIL_BATCH_H_

#include "tvgWgRenderTask.h"

class WgStencilBatch
{
public:
    bool draw(WgSceneTask* sceneTask, WgRenderDataShape* renderData, BlendMethod blendMethod, Array<WgRenderTask*>& renderTaskList);
    void clear();

private:
    WgSceneTask* sceneTask{};
    WgRenderTask* task{};
    WgRenderDataShape* first{};
    Array<RenderRegion> bounds;
    RenderRegion viewport{};
    FillRule fillRule{};
    uint32_t geometryVertexCount{};
    uint32_t geometryIndexCount{};
    bool ySorted{};

    static bool eligible(const WgRenderDataShape* renderData, BlendMethod blendMethod, RenderRegion& bounds);
    bool appendable(WgSceneTask* sceneTask, WgRenderDataShape* renderData, const RenderRegion& bounds, const Array<WgRenderTask*>& renderTaskList) const;
    bool intersects(const RenderRegion& bounds) const;
    void addBounds(const RenderRegion& bounds);
    void emitSingle(WgSceneTask* sceneTask, WgRenderDataShape* renderData, const RenderRegion& bounds, Array<WgRenderTask*>& renderTaskList);
    void promote(WgRenderDataShape* renderData, const RenderRegion& bounds, Array<WgRenderTask*>& renderTaskList);
    void append(WgRenderDataShape* renderData, const RenderRegion& bounds);
};

#endif // _TVG_WG_STENCIL_BATCH_H_
