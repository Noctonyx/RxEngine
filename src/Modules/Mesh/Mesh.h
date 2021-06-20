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
#include "DirectXCollision.h"
#include "Modules/Renderer/Renderer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/IndexBuffer.hpp"

namespace RxEngine
{
    struct MeshBundle
    {
        std::shared_ptr<RxCore::Buffer> vertexBuffer;
        std::shared_ptr<RxCore::IndexBuffer> indexBuffer;
        //std::shared_ptr<RxCore::DescriptorSet> descriptorSet;

        uint32_t vertexSize{};

        uint32_t vertexCount{};
        uint32_t indexCount{};

        uint32_t maxIndexCount{};
        uint32_t maxVertexCount{};

        //bool useDescriptor{};

        std::vector<ecs::entity_t> entries;
        uint64_t address;
    };

    struct Mesh
    {
        uint32_t vertexOffset{};
        uint32_t indexOffset{};
        uint32_t indexCount{};

        DirectX::BoundingBox boundBox;
        //DirectX::BoundingSphere boundSphere;

        std::vector<ecs::entity_t> subMeshes;
    };

    struct SubMesh
    {
        uint32_t indexOffset;
        uint32_t indexCount;
        uint32_t subMeshIndex;
    };

    struct InBundle : ecs::Relation
    {
    };

    struct SubMeshOf : ecs::Relation
    {
    };

    struct UsesMaterial : ecs::Relation
    {
    };

    struct RenderDetailCache
    {
        ecs::entity_t bundle;
        //uint32_t bundleEntry;
        ecs::entity_t shadowPipeline;
        ecs::entity_t opaquePipeline;
        ecs::entity_t transparentPipeline;
        uint32_t vertexOffset;
        uint32_t indexOffset;
        uint32_t indexCount;
        ecs::entity_t material;
        DirectX::BoundingBox boundBox;
    };

    struct InstanceBuffers
    {
        uint32_t count;
        std::vector<std::shared_ptr<RxCore::Buffer>> buffers;
        std::vector<uint32_t> sizes;
        uint32_t ix;
    };

    class MeshModule final : public Module
    {
    public:
        MeshModule(ecs::World * world, EngineMain * engine, const ecs::entity_t moduleId)
            : Module(world, engine, moduleId)
        {}

        void startup() override;
        void shutdown() override;

        static void renderIndirectDraws(ecs::World * world,
                                        IndirectDrawSet ids,
                                        const std::shared_ptr<RxCore::SecondaryCommandBuffer> & buf,
                                        uint32_t & triangles, uint32_t & drawCalls);
        static void drawInstances(std::shared_ptr<RxCore::Buffer> instanceBuffer,
                                  ecs::World * world,
                                  const GraphicsPipeline * pipeline,
                                  const PipelineLayout * const layout,
                                  IndirectDrawSet & ids);
    };
}
