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

#ifndef _TVG_WG_SOLID_BATCH_H_
#define _TVG_WG_SOLID_BATCH_H_

#include "tvgWgRenderTask.h"

class WgSolidBatch
{
public:
    bool draw(WgSceneTask* sceneTask, WgRenderDataShape* renderData, BlendMethod blendMethod, Array<WgRenderTask*>& renderTaskList);
    void clear() { *this = {}; }

private:
    WgSceneTask* sceneTask{};
    WgRenderTask* task{};
    WgRenderDataShape* first{};
    RenderRegion viewport{};
    uint32_t vertexCount{};
    uint32_t indexCount{};

    static bool eligible(const WgRenderDataShape* renderData, BlendMethod blendMethod);
    bool appendable(WgSceneTask* sceneTask, WgRenderDataShape* renderData, const Array<WgRenderTask*>& renderTaskList) const;
    void emitSingle(WgSceneTask* sceneTask, WgRenderDataShape* renderData, Array<WgRenderTask*>& renderTaskList);
    void promote(WgRenderDataShape* renderData, Array<WgRenderTask*>& renderTaskList);
    void append(WgRenderDataShape* renderData);
};

#endif  // _TVG_WG_SOLID_BATCH_H_
