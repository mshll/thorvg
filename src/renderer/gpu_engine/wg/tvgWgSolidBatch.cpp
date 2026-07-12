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

#include "tvgWgSolidBatch.h"

struct WgSolidBatchTask : public WgRenderTask
{
    Array<WgRenderDataShape*> shapes;
    WgSolidBatchRange range{};

    WgSolidBatchTask(WgRenderDataShape* first, WgRenderDataShape* second)
    {
        shapes.reserve(2);
        shapes.push(first);
        shapes.push(second);
    }

    void stage(WgCompositor& compositor) override
    {
        compositor.requestSolidBatch(shapes, range);
    }

    void run(WgContext& context, WgCompositor& compositor, WGPUCommandEncoder encoder) override
    {
        compositor.renderSolidBatch(context, range);
    }
};

bool WgSolidBatch::eligible(const WgRenderDataShape* renderData, BlendMethod blendMethod)
{
    if (!renderData || blendMethod != BlendMethod::Normal) return false;
    if (renderData->renderSettingsShape.skip || renderData->renderSettingsShape.fillType != WgRenderSettingsType::Solid) return false;
    if (!renderData->convex || renderData->viewport.invalid() || !renderData->clips.empty()) return false;
    if (renderData->meshShape.vbuffer.empty() || renderData->meshShape.ibuffer.empty()) return false;
    if (!renderData->renderSettingsStroke.skip && !renderData->meshStrokes.ibuffer.empty()) return false;
    if (renderData->meshShape.vbuffer.count > UINT32_MAX / sizeof(Point)) return false;
    if (renderData->meshShape.ibuffer.count > UINT32_MAX / sizeof(uint32_t)) return false;
    return true;
}

bool WgSolidBatch::appendable(WgSceneTask* sceneTask, WgRenderDataShape* renderData, const Array<WgRenderTask*>& renderTaskList) const
{
    // Any task submitted after the candidate is an implicit batch boundary.
    if (this->sceneTask != sceneTask) return false;
    if (sceneTask->children.empty() || sceneTask->children.last() != task) return false;
    if (renderTaskList.empty() || renderTaskList.last() != task) return false;
    if (!(viewport == renderData->viewport)) return false;

    const uint64_t nextVertexCount = static_cast<uint64_t>(vertexCount) + renderData->meshShape.vbuffer.count;
    const uint64_t nextIndexCount = static_cast<uint64_t>(indexCount) + renderData->meshShape.ibuffer.count;
    if (nextVertexCount > UINT32_MAX / sizeof(Point)) return false;
    if (nextIndexCount > UINT32_MAX / sizeof(uint32_t)) return false;
    return true;
}

void WgSolidBatch::emitSingle(WgSceneTask* sceneTask, WgRenderDataShape* renderData, Array<WgRenderTask*>& renderTaskList)
{
    auto task = new WgPaintTask(renderData, BlendMethod::Normal);
    sceneTask->children.push(task);
    renderTaskList.push(task);

    this->sceneTask = sceneTask;
    this->task = task;
    first = renderData;
    viewport = renderData->viewport;
    vertexCount = renderData->meshShape.vbuffer.count;
    indexCount = renderData->meshShape.ibuffer.count;
}

void WgSolidBatch::promote(WgRenderDataShape* renderData, Array<WgRenderTask*>& renderTaskList)
{
    assert(sceneTask && first && task);
    assert(sceneTask->children.last() == task && renderTaskList.last() == task);

    // Tasks are staged only after the tree is complete, so replacing its tail is safe.
    auto batchTask = new WgSolidBatchTask(first, renderData);
    sceneTask->children.last() = batchTask;
    renderTaskList.last() = batchTask;
    delete task;

    task = batchTask;
    first = nullptr;
    vertexCount += renderData->meshShape.vbuffer.count;
    indexCount += renderData->meshShape.ibuffer.count;
}

void WgSolidBatch::append(WgRenderDataShape* renderData)
{
    assert(sceneTask && !first && task);
    static_cast<WgSolidBatchTask*>(task)->shapes.push(renderData);
    vertexCount += renderData->meshShape.vbuffer.count;
    indexCount += renderData->meshShape.ibuffer.count;
}

bool WgSolidBatch::draw(WgSceneTask* sceneTask, WgRenderDataShape* renderData, BlendMethod blendMethod, Array<WgRenderTask*>& renderTaskList)
{
    if (!eligible(renderData, blendMethod)) return false;

    if (!appendable(sceneTask, renderData, renderTaskList)) {
        emitSingle(sceneTask, renderData, renderTaskList);
        return true;
    }

    if (first) promote(renderData, renderTaskList);
    else append(renderData);
    return true;
}
