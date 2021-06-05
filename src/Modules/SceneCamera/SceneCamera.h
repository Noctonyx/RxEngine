////////////////////////////////////////////////////////////////////////////////
// MIT License
//
// Copyright (c) 2021.  Shane Hyde
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "DirectXMath.h"
#include "Modules/Module.h"
#include "RxCore.h"

namespace RxCore {
    class Buffer;
}

namespace RxEngine
{
    class Camera;

    struct SceneCameraShaderData
    {
        DirectX::XMFLOAT4X4 projection;
        DirectX::XMFLOAT4X4 view;
        DirectX::XMFLOAT3 viewPos;
    };

    struct SceneCamera
    {
        ecs::entity_t camera;

        RxApi::UnitformDynamicBuffer camBuffer;
        //size_t bufferAlignment;
        uint32_t ix;
        SceneCameraShaderData shaderData;

        uint32_t getDescriptorOffset() const
        {
            return static_cast<uint32_t>((ix * bufferAlignment));
        }
    };

    struct SceneCameraDescriptor
    {
        
    };

    class SceneCameraModule: public Module
    {
    public:
        SceneCameraModule(ecs::World * world, EngineMain * engine, const ecs::entity_t moduleId)
            : Module(world, engine, moduleId) {}

        void startup() override;
        void shutdown() override;
    };
}
