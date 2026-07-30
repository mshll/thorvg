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

#include <algorithm>
#include <cmath>
#include <limits>
#include "tvgWgStencilBatch.h"

static bool geometryFits(uint64_t vertexCount, uint64_t indexCount)
{
    constexpr auto maxBufferBytes = std::numeric_limits<uint32_t>::max();
    return vertexCount <= maxBufferBytes / sizeof(Point) && indexCount <= maxBufferBytes / sizeof(uint32_t);
}

struct WgStencilBatchTask: public WgRenderTask
{
    // Unlike GL, which keeps prepared stencil and cover draws in separate task
    // arrays, WG keeps the source-ordered shapes. stage() packs aggregate ranges,
    // and run() emits one stencil draw then derives the cover draws without an
    // additional per-batch command array.
    Array<WgRenderDataShape*> shapes;
    WgStencilBatchRange range{};

    WgStencilBatchTask(WgRenderDataShape* first, WgRenderDataShape* second)
    {
        shapes.reserve(2);
        shapes.push(first);
        shapes.push(second);
    }

    void stage(WgCompositor& compositor) override
    {
        compositor.requestStencilBatch(shapes, range);
    }

    void run(WgContext& context, WgCompositor& compositor, WGPUCommandEncoder encoder) override
    {
        compositor.renderStencilBatch(context, shapes, range);
    }
};


void WgStencilBatch::clear()
{
    sceneTask = nullptr;
    task = nullptr;
    first = nullptr;
    bounds.clear();
    viewport = {};
    fillRule = {};
    geometryVertexCount = 0;
    geometryIndexCount = 0;
    ySorted = false;
}


bool WgStencilBatch::eligible(const WgRenderDataShape* renderData, BlendMethod blendMethod, RenderRegion& bounds)
{
    if (!renderData || blendMethod != BlendMethod::Normal) return false;
    if (renderData->renderSettingsShape.skip) return false;
    const auto fillType = renderData->renderSettingsShape.fillType;
    if (fillType != WgRenderSettingsType::Solid && fillType != WgRenderSettingsType::Linear && fillType != WgRenderSettingsType::Radial) return false;
    if (renderData->convex || renderData->viewport.invalid() || !renderData->clips.empty()) return false;
    if (renderData->meshShape.vbuffer.empty() || renderData->meshShape.ibuffer.empty() || renderData->meshBBox.vbuffer.empty() || renderData->meshBBox.ibuffer.empty()) return false;
    if (!renderData->renderSettingsStroke.skip && !renderData->meshStrokes.ibuffer.empty()) return false;

    const auto& aabb = renderData->aabb;
    if (!std::isfinite(aabb.min.x) || !std::isfinite(aabb.min.y) || !std::isfinite(aabb.max.x) || !std::isfinite(aabb.max.y)) return false;
    const auto minX = std::max(static_cast<double>(aabb.min.x), static_cast<double>(renderData->viewport.min.x));
    const auto minY = std::max(static_cast<double>(aabb.min.y), static_cast<double>(renderData->viewport.min.y));
    const auto maxX = std::min(static_cast<double>(aabb.max.x), static_cast<double>(renderData->viewport.max.x));
    const auto maxY = std::min(static_cast<double>(aabb.max.y), static_cast<double>(renderData->viewport.max.y));
    if (maxX <= minX || maxY <= minY) return false;
    bounds = {{static_cast<int32_t>(std::floor(minX)), static_cast<int32_t>(std::floor(minY))},
              {static_cast<int32_t>(std::ceil(maxX)), static_cast<int32_t>(std::ceil(maxY))}};

    return geometryFits(
        static_cast<uint64_t>(renderData->meshShape.vbuffer.count) + renderData->meshBBox.vbuffer.count,
        static_cast<uint64_t>(renderData->meshShape.ibuffer.count) + renderData->meshBBox.ibuffer.count);
}


bool WgStencilBatch::intersects(const RenderRegion& bounds) const
{
    const auto min = ySorted ? bounds.min.y : bounds.min.x;
    const auto max = ySorted ? bounds.max.y : bounds.max.x;
    ARRAY_FOREACH(p, this->bounds) {
        const auto pMin = ySorted ? (*p).min.y : (*p).min.x;
        if ((ySorted ? (*p).max.y : (*p).max.x) <= min) continue;
        if (pMin >= max) break;
        if ((*p).intersected(bounds)) return true;
    }
    return false;
}


void WgStencilBatch::addBounds(const RenderRegion& bounds)
{
    auto p = this->bounds.count;
    const auto min = ySorted ? bounds.min.y : bounds.min.x;
    this->bounds.push(bounds);
    while (p > 0 && (ySorted ? this->bounds[p - 1].min.y : this->bounds[p - 1].min.x) > min) {
        this->bounds[p] = this->bounds[p - 1];
        --p;
    }
    this->bounds[p] = bounds;
}


bool WgStencilBatch::appendable(WgSceneTask* sceneTask, WgRenderDataShape* renderData, const RenderRegion& bounds, const Array<WgRenderTask*>& renderTaskList) const
{
    if (this->sceneTask != sceneTask) return false;
    if (sceneTask->children.empty() || sceneTask->children.last() != task) return false;
    if (renderTaskList.empty() || renderTaskList.last() != task) return false;
    if (!(viewport == renderData->viewport) || fillRule != renderData->fillRule) return false;
    if (intersects(bounds)) return false;

    return geometryFits(
        static_cast<uint64_t>(geometryVertexCount) + renderData->meshShape.vbuffer.count + renderData->meshBBox.vbuffer.count,
        static_cast<uint64_t>(geometryIndexCount) + renderData->meshShape.ibuffer.count + renderData->meshBBox.ibuffer.count);
}


void WgStencilBatch::emitSingle(WgSceneTask* sceneTask, WgRenderDataShape* renderData, const RenderRegion& bounds, Array<WgRenderTask*>& renderTaskList)
{
    auto task = new WgPaintTask(renderData, BlendMethod::Normal);
    sceneTask->children.push(task);
    renderTaskList.push(task);

    this->sceneTask = sceneTask;
    this->task = task;
    first = renderData;
    viewport = renderData->viewport;
    fillRule = renderData->fillRule;
    geometryVertexCount = renderData->meshShape.vbuffer.count + renderData->meshBBox.vbuffer.count;
    geometryIndexCount = renderData->meshShape.ibuffer.count + renderData->meshBBox.ibuffer.count;
    ySorted = viewport.sh() > viewport.sw();
    this->bounds.clear();
    addBounds(bounds);
}


void WgStencilBatch::promote(WgRenderDataShape* renderData, const RenderRegion& bounds, Array<WgRenderTask*>& renderTaskList)
{
    assert(sceneTask && first && task);
    assert(sceneTask->children.last() == task && renderTaskList.last() == task);

    auto batchTask = new WgStencilBatchTask(first, renderData);
    sceneTask->children.last() = batchTask;
    renderTaskList.last() = batchTask;
    delete task;

    task = batchTask;
    first = nullptr;
    geometryVertexCount += renderData->meshShape.vbuffer.count + renderData->meshBBox.vbuffer.count;
    geometryIndexCount += renderData->meshShape.ibuffer.count + renderData->meshBBox.ibuffer.count;
    addBounds(bounds);
}


void WgStencilBatch::append(WgRenderDataShape* renderData, const RenderRegion& bounds)
{
    assert(sceneTask && !first && task);
    static_cast<WgStencilBatchTask*>(task)->shapes.push(renderData);
    geometryVertexCount += renderData->meshShape.vbuffer.count + renderData->meshBBox.vbuffer.count;
    geometryIndexCount += renderData->meshShape.ibuffer.count + renderData->meshBBox.ibuffer.count;
    addBounds(bounds);
}


bool WgStencilBatch::draw(WgSceneTask* sceneTask, WgRenderDataShape* renderData, BlendMethod blendMethod, Array<WgRenderTask*>& renderTaskList)
{
    RenderRegion bounds{};
    if (!eligible(renderData, blendMethod, bounds)) return false;

    if (!appendable(sceneTask, renderData, bounds, renderTaskList)) {
        emitSingle(sceneTask, renderData, bounds, renderTaskList);
        return true;
    }

    if (first) promote(renderData, bounds, renderTaskList);
    else append(renderData, bounds);
    return true;
}
