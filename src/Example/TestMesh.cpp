#include "TestMesh.hpp"

#include <filesystem>

namespace RxEngine
{
#if 0
    void TestMesh::RendererInit(const RXCore::Renderer * renderer)
    {
        uboCamera_ = RXCore::VulkanContext::Context()->CreateBuffer(
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent, sizeof(UboScene)
        );
        uboCamera_->Map();

        uboModelMatrix_ = RXCore::VulkanContext::Context()->CreateBuffer(
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent, sizeof(glm::mat4)
        );
        uboModelMatrix_->Map();

        //auto cam = scene_->camera_;

        //        scene_->camera_ = std::make_shared<Camera>();

        //camera_->type = Camera::CameraType::firstperson;
        //scene_->camera_->setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
        //scene_->camera_->setTranslation(glm::vec3(0.0f, .0f, 10.0f));

        CreateMeshBuffers();

        auto res = renderer->GetSequenceRenderStage(RXCore::RenderSequence::RenderSequence_General);
        BuildPipeline(res.first);

        //std::shared_ptr<Material> mb = std::make_shared<Material>();

        //mb->SetSequencePipeline(RXCore::RenderSequence_General, trianglePipeline_);
        auto & rm = rm_.emplace_back(std::make_shared<RenderMesh>());
        rm->transform = glm::mat4(1.0f);
        auto & md = rm->meshDetails.emplace_back();
        md.distance = 0.f;
        md.lod = {triangleMaterial, mesh_};
    }

    void TestMesh::Startup(const std::vector<Subsystem *> & subsystems)
    {
        for (auto * subsystem: subsystems) {
            auto * s = dynamic_cast<Stats *>(subsystem);
            auto * camera = dynamic_cast<SceneCamera *>(subsystem);

            if (camera) {
                sceneCamera_ = camera->GetCamera();
            }
            if (s) {
                stats_ = s;
            }
        }
    }

    void TestMesh::Update(float delta)
    {
        //auto height = scene_->Window()->GetHeight();
        //auto width = scene_->Window()->GetWidth();

        //scene_->camera_->setPerspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 0.1f, 512.0f);

        // xRot_ = 90.f;
        yRot_ += 180.f * delta * 0.f;
        if (yRot_ > 360.f) {
            yRot_ -= 360.0f;
        }

        xRot_ += 28.5f * delta * 0.f;
        if (xRot_ > 360.f) {
            xRot_ -= 360.0f;
        }

        zRot_ += 58.5f * delta * 0.f;
        if (zRot_ > 360.f) {
            zRot_ -= 360.0f;
        }

        modelMat = glm::translate(position);
        modelMat = glm::scale(modelMat, glm::vec3(2.f));

        modelMat = glm::rotate(modelMat, glm::radians(xRot_), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMat = glm::rotate(modelMat, glm::radians(yRot_), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMat = glm::rotate(modelMat, glm::radians(zRot_), glm::vec3(0.0f, 0.0f, 1.0f));

        uboScene.projection = sceneCamera_->matrices.perspective;
        uboScene.view = sceneCamera_->matrices.view;
        uboScene.viewPos = sceneCamera_->basePosition;

        //uboScene.view = glm::scale(camera_->matrices.view, glm::vec3(-1.f , 1.f, -1.f));

        // auto f = Frustum(uboScene.projection * uboScene.view);
    }

    void TestMesh::UpdateGui()
    {
        bool open = true;

        ImGui::Begin("Example", &open);
        ImGui::SetNextItemWidth(250);
        ImGui::SliderFloat3("Position", (float *) (&position), -10.f, 10.f);

        ImGui::End();
    }

    void TestMesh::BindMaterial(std::shared_ptr<RXCore::SecondaryCommandBuffer> buf3,
                                RXCore::RenderSequence sequence,
                                std::shared_ptr<Material> mb) const
    {
        auto mpi = mb->getSequencePipeline(sequence);
        auto ds = mpi->getDescriptorSet();
        if (ds) {
            buf3->BindDescriptorSet(mpi->getLayout(), mpi->firstSetIndex_,
                                    mpi->getDescriptorSet());
        }
    }
#if 0
    void TestMesh::DrawMesh(std::shared_ptr<RXCore::SecondaryCommandBuffer> buf3,
                            std::shared_ptr<Mesh> mesh) const
    {
        buf3->PushBuffer(*trianglePipeline_->GetLayout(), 1, mesh->GetVertexBuffer(),
                         vk::DescriptorType::eStorageBuffer);

        buf3->BindIndexBuffer(mesh_->GetIndexBuffer());
        {
            OPTICK_GPU_EVENT("Draw")
            for (uint16_t i = 0; i < mesh->GetSubMeshCount(); i++) {
                auto sm = mesh->GetSubMesh(i);
                buf3->DrawIndexed(
                    sm.indexCount,
                    1,
                    sm.firstIndex,
                    0,
                    0);
            }
        }
    }
#endif
#if 0
    void TestMesh::SetCameraData(std::shared_ptr<RXCore::SecondaryCommandBuffer> buf3) const
    {
        buf3->PushBuffer(*trianglePipeline_->GetLayout(), 0, uboCamera_, vk::DescriptorType::eUniformBuffer);
    }
#endif
    void TestMesh::DrawAllMeshes(RXCore::RenderSequence sequence,
                                 std::shared_ptr<RXCore::SecondaryCommandBuffer> buf3) const
    {
        for (auto render_mesh: rm_) {
            if (render_mesh->meshDetails.size() > 0) {
                auto & md = render_mesh->meshDetails[0];

                auto & lod = md.lod;
                auto mb = lod.mb;
                auto mesh = lod.mesh;
                auto pl = mb->GetPipeline(sequence);

                buf3->BindPipeline(pl->getPipeline());
#if 0
                buf3->PushBuffer(*(pl->getLayout()), 0, uboCamera_,
                                 vk::DescriptorType::eUniformBuffer);
#endif
                BindMaterial(buf3, sequence, mb);

                buf3->PushConstant(*(pl->getLayout()), vk::ShaderStageFlagBits::eVertex, 0,
                                   sizeof(glm::mat4),
                                   static_cast<const void *>(&modelMat));
                //buf3->PushBuffer(*trianglePipeline_->GetLayout(), 2, ubo2_, vk::DescriptorType::eUniformBuffer);

                //DrawMesh(buf3, mesh);
#if 0
                buf3->PushBuffer(*(pl->getLayout()), 1, mesh->GetVertexBuffer(),
                                 vk::DescriptorType::eStorageBuffer);
#endif
                buf3->BindIndexBuffer(mesh->GetIndexBuffer());
                {
                    OPTICK_GPU_EVENT("Draw")
                    for (uint16_t i = 0; i < mesh->GetSubMeshCount(); i++) {
                        auto sm = mesh->GetSubMesh(i);
                        buf3->DrawIndexed(sm.indexCount, 1, sm.firstIndex, 0, 0);
                    }
                }
            }
        }
    }

    RXCore::RenderResponse TestMesh::Render(
        RXCore::RenderSequence sequence,
        const RXCore::RenderStage & stage) const
    {
        if (sequence != RXCore::RenderSequence_General) {
            return {};
        }

        uboCamera_->UpdateBuffer(&uboScene, sizeof(UboScene));
        uboModelMatrix_->UpdateBuffer(&modelMat, sizeof(glm::mat4));

        auto buf3 = RXCore::JobManager::GetCommandBuffer(stage);

        buf3->Begin();
        {
            OPTICK_GPU_CONTEXT(buf3->Handle())
            OPTICK_GPU_EVENT("DoMesh")

            buf3->SetScissor({{0, 0}, {viewPortWidth_, viewPortHeight_}});
            buf3->SetViewport(
                .0f,
                static_cast<float>(viewPortHeight_),
                static_cast<float>(viewPortWidth_),
                -static_cast<float>(viewPortHeight_),
                0.0f,
                1.0f);
            {
                OPTICK_GPU_EVENT("Bind PIpeline")
                //buf3->BindPipeline(trianglePipeline_);
            }
            {
                OPTICK_GPU_EVENT("Bind Buffers and Draw")
                //SetCameraData(buf3);
                DrawAllMeshes(sequence, buf3);
            }
        }
        buf3->End();
        return {buf3};
    }

    void TestMesh::Shutdown()
    {
        uboCamera_.reset();
        mesh_.reset();
        triangleMaterial.reset();
        //trianglePipeline_.reset();
    }

    void TestMesh::CreateMeshBuffers()
    {
        mesh_ = AssetCache::Instance()->LoadMesh("test1");

#if 0
        std::filesystem::path obj(RX_ASSET_DIR);
        obj.append("axes.obj");
        mesh_ = std::make_shared<Mesh>();
        mesh_->Load(obj.generic_string().c_str());
#endif
    }

    void TestMesh::BuildPipeline(std::shared_ptr<RXCore::RenderPass> & renderPass)
    {
        RXCore::PipelineLayoutBuilder plb;

        auto mp = std::make_shared<MaterialPipeline>();

        //RXCore::PipelineLayoutBuilder lb;
        std::vector<vk::DescriptorSetLayoutBinding> dslb = {
            {
                0, vk::DescriptorType::eUniformBuffer, 1,
                vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, nullptr
            },
            {1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr},
            // {2, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr},
        };
        plb.addPushConstantRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4));
        plb.addDescriptorSetLayout(vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR, dslb);

        auto plo = plb.build();

        //auto layout = lb.Build();

        //RXCore::PipelineBuilder pb(layout);

        mp->addAttachmentColorBlending(false);
        //pb.SetCullMode(EPipelineCullMode::None);

        auto vs = RXEngine::AssetCache::Instance()->loadShader("../shaders/triangle.vert.spv");
        auto ps = RXEngine::AssetCache::Instance()->loadShader("../shaders/triangle.frag.spv");
        mp->addShader(vs, vk::ShaderStageFlagBits::eVertex);
        mp->addShader(ps, vk::ShaderStageFlagBits::eFragment);

        mp->setDepthMode(true, true);
        mp->build(plo, renderPass, 0);

        triangleMaterial = std::make_shared<Material>();
        triangleMaterial->SetSequencePipeline(RXCore::RenderSequence_General, mp->createInstance());
        //pb.SetLineMode();

        //        trianglePipeline_ = pb.Build(renderPass, 0);
    }
#endif
} // namespace RXCore
