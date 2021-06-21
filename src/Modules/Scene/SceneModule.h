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

//
// Created by shane on 20/06/2021.
//

#ifndef INDUSTRONAUT_SCENEMODULE_H
#define INDUSTRONAUT_SCENEMODULE_H

#include <Modules/Module.h>
#include "RxECS.h"
#include <DirectXMath.h>
#include <DirectXCollision.h>

namespace RxEngine
{
    struct SceneNode
    {
        //bool dirty;
        ecs::entity_t  parent{};
        std::vector<ecs::entity_t> children{};

        static void parentEntity(ecs::EntityHandle parent, ecs::EntityHandle child);
    };

    struct LocalPosition
    {
        DirectX::XMFLOAT3 position;
    };

    struct LocalRotation
    {
        DirectX::XMFLOAT3 rotation;
    };

    struct LocalScale
    {
        float scale;
    };

    struct WorldPosition
    {
        DirectX::XMFLOAT3 position;
    };

    struct WorldRotation
    {
        DirectX::XMFLOAT3 rotation;
    };

    struct WorldScale
    {
        float scale;
    };

    struct WorldTransform
    {
        DirectX::XMFLOAT4X4 transform;
    };

    struct LocalBoundingBox
    {
        DirectX::BoundingBox boundBox;
    };

    struct WorldBoundingSphere
    {
        DirectX::BoundingSphere boundSphere;
    };

    class SceneModule final : public Module
    {
    public:
        SceneModule(ecs::World * world, EngineMain * engine, ecs::entity_t moduleId);
        void startup() override;
        void shutdown() override;

    protected:
        bool updatedSceneNode(ecs::EntityHandle e);
        static bool updateTransforms(ecs::EntityHandle e);

        ecs::EntityQueueHandle updateTransformQueue{};
    };
}

#endif //INDUSTRONAUT_SCENEMODULE_H
