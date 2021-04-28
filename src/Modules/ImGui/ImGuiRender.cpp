//
// Created by shane on 3/04/2020.
//

#include <cinttypes>
#include <imgui.h>
#include <memory>
#include "ImGuiRender.hpp"
#include "RxCore.h"
#include "Window.hpp"
#include "Scene.h"
#include "AssetException.h"
#include "Vfs.h"

namespace RxEngine
{
    IMGuiRender::IMGuiRender(ecs::World * world, EngineMain * engine)
        : Module(world, engine)
        , window_(engine->getWindow())
    {
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGuiIO & io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        setupInputs(io);
    };

    IMGuiRender::~IMGuiRender()
    {
        fontImage_.reset();
        set0_.reset();
    }
    
    void IMGuiRender::startup()
    {
        auto wd = world_->getSingleton<WindowDetails>();
        assert(wd);
        ImGuiIO & io = ImGui::GetIO();
        io.DisplaySize = ImVec2(
            static_cast<float>(wd->width),
            static_cast<float>(wd->height));

        world_->createSystem("ImGui:UpdateGui")
              //.after<UiContextUpdate>()
              .inGroup("Pipeline:UpdateUi")
              .execute([&, this](ecs::World * world)
              {
                  OPTICK_EVENT("ImGui:UpdateGui")
                  this->updateGui();
              });

        world_->createSystem("ImGui:WindowResize")
              .withStream<WindowResize>()
              .inGroup("Pipeline:PreFrame")
              //.label<UiContextUpdate>()
              .execute<WindowResize>([](ecs::World * world, const WindowResize * resize)
              {
                  OPTICK_EVENT("ImGui:WindowResize")

                  ImGuiIO & io = ImGui::GetIO();
                  io.DisplaySize = ImVec2(
                      static_cast<float>(resize->width),
                      static_cast<float>(resize->height));
                  return false;
              });

        world_->createSystem("ImGui:MousePos")
              .withStream<MousePosition>()
              .inGroup("Pipeline:PreFrame")
              .execute<MousePosition>([](ecs::World * world, const MousePosition * pos)
              {
                  OPTICK_EVENT("ImGui:MousePos")
                  auto & io = ImGui::GetIO();
                  io.MousePos = ImVec2(pos->x, pos->y);
                  if (io.WantCaptureMouse) {
                      return true;
                  }
                  return false;
              });

        world_->createSystem("ImGui:MouseButton")
              .withStream<MouseButton>()
              .inGroup("Pipeline:PreFrame")
              .execute<MouseButton>([](ecs::World * world, const MouseButton * button)
              {
                  OPTICK_EVENT()
                  auto & io = ImGui::GetIO();
                  if (io.WantCaptureMouse) {
                      io.MouseDown[static_cast<int>(button->button)] = button->pressed;
                      return true;
                  }
                  return false;
              });

        world_->createSystem("ImGui:MouseScroll")
              .withStream<MouseScroll>()
              .inGroup("Pipeline:PreFrame")
              .execute<MouseScroll>([](ecs::World * world, const MouseScroll * s)
              {
                  OPTICK_EVENT()
                  auto & io = ImGui::GetIO();
                  if (io.WantCaptureMouse) {
                      io.MouseWheel += s->y_offset;
                      return true;
                  }
                  return false;
              });

        world_->createSystem("ImGui:Key")
              .withStream<KeyboardKey>()
              .inGroup("Pipeline:PreFrame")
              .execute<KeyboardKey>([](ecs::World * world, const KeyboardKey * key)
              {
                  OPTICK_EVENT()
                  auto & io = ImGui::GetIO();
                  if (!io.WantCaptureKeyboard) {
                      return false;
                  }
                  if (key->action == EInputAction::Press) {
                      io.KeysDown[static_cast<int>(key->key)] = true;
                  }
                  if (key->action == EInputAction::Release) {
                      io.KeysDown[static_cast<int>(key->key)] = false;
                  }
                  io.KeyCtrl = io.KeysDown[static_cast<int>(EKey::ControlLeft)] ||
                      io.KeysDown[static_cast<int>(EKey::ControlRight)];
                  io.KeyShift = io.KeysDown[static_cast<int>(EKey::ShiftLeft)] ||
                      io.KeysDown[static_cast<int>(EKey::ShiftRight)];
                  io.KeyAlt = io.KeysDown[static_cast<int>(EKey::AltLeft)] ||
                      io.KeysDown[static_cast<int>(EKey::AltRight)];
                  io.KeySuper = io.KeysDown[static_cast<int>(EKey::SuperLeft)] ||
                      io.KeysDown[static_cast<int>(EKey::SuperRight)];
                  return true;
              });

        world_->createSystem("ImGui:Char")
              .withStream<KeyboardChar>()
              .inGroup("Pipeline:PreFrame")
              .execute<KeyboardChar>([](ecs::World * world, const KeyboardChar * c)
              {
                  OPTICK_EVENT("ImGui:Char")
                  auto & io = ImGui::GetIO();
                  if (!io.WantCaptureKeyboard) {
                      return false;
                  }
                  io.AddInputCharacter(c->c);
                  return true;
              });

        world_->createSystem("ImGui:NewFrame")
              //.after<UiContextUpdate>()
              .inGroup("Pipeline:PreFrame")
              .execute([this](ecs::World * world)
              {
                  OPTICK_EVENT("ImGui:NewFrame")
                  update(world->deltaTime());
              });

        world_->createSystem("Imgui:Render")
              .inGroup("Pipeline:PreRender")
              .execute([this](ecs::World *)
              {
                  OPTICK_EVENT("Imgui:Render")
                  createRenderCommands();
              });

        if (!fontImage_) {
            createFontImage(io);
        }

        pipeline_ = world_->lookup("pipeline/imgui");

        createDescriptorSet();
    }

    void IMGuiRender::shutdown() { }

    void IMGuiRender::setupInputs(ImGuiIO & io)
    {
        io.KeyRepeatDelay = 2.0f;
        io.KeyRepeatRate = 0.25f;
        io.KeyMap[ImGuiKey_Tab] = static_cast<int>(EKey::Tab);
        io.KeyMap[ImGuiKey_LeftArrow] = static_cast<int>(EKey::Left);
        io.KeyMap[ImGuiKey_RightArrow] = static_cast<int>(EKey::Right);
        io.KeyMap[ImGuiKey_UpArrow] = static_cast<int>(EKey::Up);
        io.KeyMap[ImGuiKey_DownArrow] = static_cast<int>(EKey::Down);
        io.KeyMap[ImGuiKey_PageUp] = static_cast<int>(EKey::PageUp);
        io.KeyMap[ImGuiKey_PageDown] = static_cast<int>(EKey::PageDown);
        io.KeyMap[ImGuiKey_Home] = static_cast<int>(EKey::Home);
        io.KeyMap[ImGuiKey_End] = static_cast<int>(EKey::End);
        io.KeyMap[ImGuiKey_Insert] = static_cast<int>(EKey::Insert);
        io.KeyMap[ImGuiKey_Delete] = static_cast<int>(EKey::Delete);
        io.KeyMap[ImGuiKey_Backspace] = static_cast<int>(EKey::Backspace);
        io.KeyMap[ImGuiKey_Space] = static_cast<int>(EKey::Space);
        io.KeyMap[ImGuiKey_Enter] = static_cast<int>(EKey::Enter);
        io.KeyMap[ImGuiKey_Escape] = static_cast<int>(EKey::Escape);
        io.KeyMap[ImGuiKey_A] = static_cast<int>(EKey::A);
        io.KeyMap[ImGuiKey_C] = static_cast<int>(EKey::C);
        io.KeyMap[ImGuiKey_V] = static_cast<int>(EKey::V);
        io.KeyMap[ImGuiKey_X] = static_cast<int>(EKey::X);
        io.KeyMap[ImGuiKey_Y] = static_cast<int>(EKey::Y);
        io.KeyMap[ImGuiKey_Z] = static_cast<int>(EKey::Z);

        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    }

    void IMGuiRender::createFontImage(ImGuiIO & io)
    {
        unsigned char * pixels;
        int width, height;

        auto vfs = RxAssets::Vfs::getInstance();

        auto size = vfs->getFilesize("/fonts/NotoSans-Regular.ttf");
        if (size == 0) {
            throw RxAssets::AssetException("Error loading font /fonts/NotoSans-Regular.ttf",
                                           std::string(""));
        }

        std::vector<std::byte> data(size);
        vfs->getFileContents("/fonts/NotoSans-Regular.ttf", data.data());

        io.Fonts->AddFontFromMemoryTTF(data.data(), static_cast<uint32_t>(data.size()), 16);
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        const size_t upload_size = width * height * 4 * sizeof(char);

        auto d = std::make_unique<uint8_t[]>(upload_size);
        memcpy(d.get(), pixels, upload_size);

        fontImage_ = RxCore::Device::Context()->createImage(
            vk::Format::eR8G8B8A8Unorm,
            {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1}, 1, 1,
            vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);

        auto b = RxCore::iVulkan()->createStagingBuffer(upload_size, pixels);

        RxCore::iVulkan()->transferBufferToImage(
            b, fontImage_, vk::Extent3D(width, height, 1), vk::ImageLayout::eShaderReadOnlyOptimal,
            1,
            0);
    }

    void IMGuiRender::createDescriptorSet()
    {
        auto layout = pipeline_.getRelated<UsesLayout, PipelineLayout>();

        auto dp = RxCore::Device::Context()->CreateDescriptorPool(
            {{vk::DescriptorType::eCombinedImageSampler, 1}}, 1);

        set0_ = dp->allocateDescriptorSet(layout->dsls[0]);

        vk::SamplerCreateInfo sci;
        sci.setMagFilter(vk::Filter::eLinear)
           .setMinFilter(vk::Filter::eLinear)
           .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
           .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
           .setMaxLod(1.0f)
           .setBorderColor(vk::BorderColor::eFloatOpaqueWhite);

        auto sampler = RxCore::iVulkan()->createSampler(sci);

        set0_->updateDescriptor(0, vk::DescriptorType::eCombinedImageSampler, fontImage_, sampler);
    }

    void IMGuiRender::update(float deltaTime)
    {
        OPTICK_EVENT()

        auto & io = ImGui::GetIO();
        io.DeltaTime = deltaTime;

        if (!(io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)) {
            ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
            if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor) {
                window_->hideCursor(true);
            } else {
                window_->hideCursor(false);
                switch (imgui_cursor) {
                default:
                case ImGuiMouseCursor_ResizeAll:
                case ImGuiMouseCursor_ResizeNESW:
                case ImGuiMouseCursor_ResizeNWSE:
                    window_->setCursor(ECursorStandard::Arrow);
                    break;
                case ImGuiMouseCursor_TextInput:
                    window_->setCursor(ECursorStandard::IBeam);
                    break;
                case ImGuiMouseCursor_ResizeNS:
                    window_->setCursor(ECursorStandard::ResizeY);
                    break;
                case ImGuiMouseCursor_ResizeEW:
                    window_->setCursor(ECursorStandard::ResizeX);
                    break;
                case ImGuiMouseCursor_Hand:
                    window_->setCursor(ECursorStandard::Hand);
                    break;
                }
            }
        }

        ImGui::NewFrame();
    }

    auto createBuffers()
    {
        OPTICK_EVENT("Build IMGui Buffers")

        const auto dd = ImGui::GetDrawData();

        auto vb =
            RxCore::iVulkan()->createVertexBuffer(
                VMA_MEMORY_USAGE_CPU_TO_GPU, dd->TotalVtxCount,
                static_cast<uint32_t>(sizeof(ImDrawVert)));

        auto ib = RxCore::iVulkan()->createIndexBuffer(
            VMA_MEMORY_USAGE_CPU_TO_GPU,
            dd->TotalIdxCount, true);

        vb->getMemory()->map();
        ib->getMemory()->map();

        uint32_t vbOffset = 0;
        uint32_t idxOffset = 0;

        for (auto n = 0; n < dd->CmdListsCount; n++) {
            auto cmdList = dd->CmdLists[n];

            vb->getMemory()->update(
                cmdList->VtxBuffer.Data, vbOffset,
                cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
            ib->getMemory()->update(
                cmdList->IdxBuffer.Data, idxOffset,
                cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));

            vbOffset += cmdList->VtxBuffer.Size * sizeof(ImDrawVert);
            idxOffset += cmdList->IdxBuffer.Size * sizeof(ImDrawIdx);
        }

        vb->getMemory()->unmap();
        ib->getMemory()->unmap();

        return std::pair(vb, ib);
    }

    void IMGuiRender::createRenderCommands()
    {
        OPTICK_CATEGORY("Render UI", ::Optick::Category::UI)

        ImGuiIO & io = ImGui::GetIO();

        auto wd = world_->getSingleton<WindowDetails>();

        io.DisplaySize = ImVec2(static_cast<float>(wd->width), static_cast<float>(wd->height));
        ImGui::Render();

        auto pipeline = pipeline_.get<UiPipeline>();

        if (!pipeline) {
            return;
        }
        assert(pipeline);
        assert(pipeline->pipeline);

        const auto layout = pipeline_.getRelated<UsesLayout, PipelineLayout>();

        auto buf = RxCore::JobManager::threadData().getCommandBuffer();

        const auto dd = ImGui::GetDrawData();
        if (dd->TotalVtxCount == 0) {
            return;
        }

        // Create Vertex/Index Buffers
        auto [vb, ib] = createBuffers();

        buf->begin(pipeline->renderPass, pipeline->subPass);
        {
            buf->useLayout(layout->layout);
            OPTICK_GPU_CONTEXT(buf->Handle());
            OPTICK_GPU_EVENT("Draw IMGui");

            buf->BindPipeline(pipeline->pipeline->Handle());
            buf->BindDescriptorSet(0, set0_);
            struct
            {
                DirectX::XMFLOAT2 scale;
                DirectX::XMFLOAT2 translate;
            } pd{};

            auto scale = DirectX::XMFLOAT2(2.0f / dd->DisplaySize.x, 2.0f / dd->DisplaySize.y);
            auto translate =
                DirectX::XMFLOAT2(-1.0f - dd->DisplayPos.x * scale.x,
                                  -1.0f - dd->DisplayPos.y * scale.y);

            pd.translate = translate;
            pd.scale = scale;

            buf->pushConstant(
                vk::ShaderStageFlagBits::eVertex, 0,
                sizeof(pd), static_cast<void *>(&pd));

            buf->BindVertexBuffer(vb);
            buf->BindIndexBuffer(ib);
            buf->setViewport(0, 0, dd->DisplaySize.x, dd->DisplaySize.y, 0, 1);

            uint32_t vb_offset = 0;
            uint32_t idx_offset = 0;

            for (auto n = 0; n < dd->CmdListsCount; n++) {
                auto cmd_list = dd->CmdLists[n];

                for (auto j = 0; j < cmd_list->CmdBuffer.Size; j++) {
                    auto draw_cmd = &cmd_list->CmdBuffer[j];
                    buf->setScissor(
                        {
                            {
                                std::max(0, static_cast<int32_t>(draw_cmd->ClipRect.x)),
                                std::max(0, static_cast<int32_t>(draw_cmd->ClipRect.y))
                            },
                            {
                                static_cast<uint32_t>(draw_cmd->ClipRect.z - draw_cmd->ClipRect.x),
                                static_cast<uint32_t>(draw_cmd->ClipRect.w - draw_cmd->ClipRect.y)
                            }
                        });
                    buf->DrawIndexed(
                        draw_cmd->ElemCount, 1, draw_cmd->IdxOffset + idx_offset,
                        draw_cmd->VtxOffset + vb_offset, 0);
                }

                vb_offset += cmd_list->VtxBuffer.Size;
                idx_offset += cmd_list->IdxBuffer.Size;
            }
        }
        buf->end();

        world_->getStream<Render::UiRenderCommand>()
              ->add<Render::UiRenderCommand>({buf});
    }

    void IMGuiRender::updateGui()
    {
        ImGui::DockSpaceOverViewport(
            0, ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoDockingInCentralNode);
#if 1
        if (showDemoWindow_) {
            ImGui::ShowDemoWindow(&showDemoWindow_);
        }

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a
        // named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into
            // it.

            ImGui::Text("This is some useful text."); // Display some text (you can use a format
            // strings too)
            ImGui::Checkbox("Demo Window", &showDemoWindow_); // Edit bools storing our window
            // open/close state
            ImGui::Checkbox("Another Window", &showAnotherWindow_);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f); // Edit 1 float using a slider from 0.0f
            // to 1.0f
            // ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing
            // a color

            if (ImGui::Button("Button")) {
                // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            }
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text(
                "Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
            ImGui::End();
        }
#endif
    }
} // namespace RXEngine
