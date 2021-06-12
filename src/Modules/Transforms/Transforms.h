////////////////////////////////////////////////////////////////////////////////
// MIT License
//
// Copyright (c) 2021.  Shane Hyde (shane@noctonyx.com)
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

#include "Modules/Module.h"
#include "RxECS.h"
#include "DirectXCollision.h"
#include "EngineMain.hpp"

struct Sector;

namespace RxEngine
{
    namespace Transforms
    {
        struct WorldPosition
        {
            DirectX::XMFLOAT3 position;
        };

        struct LocalRotation
        {
            DirectX::XMFLOAT3 rotation;
        };
#if 0
        struct YRotation
        {
            float yRotation;
        };

        struct XRotation
        {
            float xRotation;
        };
#endif
        struct ScalarScale
        {
            float scale;
        };

        struct LocalBoundingSphere
        {
            DirectX::BoundingSphere boundSphere;
        };

        struct WorldBoundingSphere
        {
            DirectX::BoundingSphere boundSphere;
        };

        struct LocalBoundingBox
        {
            DirectX::BoundingBox boundBox;
        };
    }

    class TransformsModule : public Module
    {
    public:
        TransformsModule(ecs::World * world, EngineMain * engine, const ecs::entity_t moduleId)
            : Module(world, engine, moduleId)
        {}

        void startup() override;
        void shutdown() override;

    private:

        //static void worldPositionGui(void * ptr);
        //static void yRotationGui(void* ptr);
        //static void xRotationGui(void* ptr);
        //static void scalarScaleGui(void* ptr);
    };
};
