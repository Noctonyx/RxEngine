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
#include <Modules/StaticMesh/StaticMesh.h>
#include "Modules/Module.h"
#include "DirectXCollision.h"
#include "Modules/Renderer/Renderer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/IndexBuffer.hpp"

namespace RxEngine
{
    struct DynamicMeshActiveBundle
    {
        ecs::entity_t currentBundle = 0;
    };

#pragma warning(push)
#pragma warning(disable: 4324)
    struct DynamicMeshVertex
    {
        alignas(16) DirectX::XMFLOAT3 point;
        alignas(16) DirectX::XMFLOAT3 normal;
        alignas(16) DirectX::XMFLOAT2 uv;

        DynamicMeshVertex(const DirectX::XMFLOAT3 & pos,
                          const DirectX::XMFLOAT3 & normal,
                          const DirectX::XMFLOAT2 & uv)
            : point(pos)
            , normal(normal)
            , uv(uv) {}
    };
#pragma warning(pop)

    struct DynamicInstance
    {
        ecs::entity_t pipeline;
        ecs::entity_t bundle;
        uint32_t vertexOffset;
        uint32_t indexOffset;
        uint32_t indexCount;
        ecs::entity_t material;
        uint32_t matrixIndex;
        //DirectX::XMFLOAT4X4 mat;
    };

    struct DynamicSubMeshEntry
    {
        uint32_t firstIndex;
        uint32_t indexCount;
        ecs::entity_t materialId;
    };

    struct DynamicMesh {};

    struct DynamicInstanceBuffers
    {
        uint32_t count;
        std::vector<std::shared_ptr<RxCore::Buffer>> buffers;
        //std::vector<std::shared_ptr<RxCore::DescriptorSet>> descriptorSets;
        std::vector<uint32_t> sizes;

        uint32_t ix;
    };

    class DynamicMeshModule : public Module
    {
    public:
        DynamicMeshModule(ecs::World * world, EngineMain * engine, const ecs::entity_t moduleId)
            : Module(world, engine, moduleId) {}

        void startup() override;
        void shutdown() override;

        ecs::EntityHandle createDynamicMeshObject(ecs::World * world,
                                                  RxCore::Device * device,
                                                  const std::vector<DynamicMeshVertex> & vertices,
                                                  const std::vector<uint32_t> & indices,
                                                  const std::vector<DynamicSubMeshEntry> & submeshes
        );
    protected:
        void createOpaqueRenderCommands();
        void renderIndirectDraws(IndirectDrawSet ids,
                                 const std::shared_ptr<RxCore::SecondaryCommandBuffer> & buf) const;

    private:
        ecs::EntityHandle pipeline_{};
        ecs::queryid_t worldObjects_{};

        std::vector<StaticInstance> instances{};
        std::vector<DirectX::XMFLOAT4X4> mats{};
    };
}
