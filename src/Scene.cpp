#include <algorithm>

#include "Scene.h"
#include "Subsystem.h"
#include "Window.hpp"
#include "Input/Mouse.hpp"
#include "Input/Keyboard.hpp"
#include "Subsystems/IInputHandler.h"

namespace RxEngine
{
    void Scene::Startup(Renderer * renderer)
    {
        deltaAccumulated_ = 0.f;

        for (int i = 0; i < 9; ++i) {
            CreateSubsystemSequence(i);
        }
#if 0
        std::vector<Subsystem *> ss(subsystems_.size());

        std::ranges::transform(subsystems_, ss.begin(), [](std::shared_ptr<Subsystem> & x)
        {
            return x.get();
        });
        for (auto * s: subsystemSequences_[Sequences::Startup]) {
            s->Startup(ss);
        }

#endif
        for (auto * s: subsystemSequences_[static_cast<uint8_t>(Sequences::RendererInit)]) {
            auto r = dynamic_cast<IRenderable *>(s);
            if (r) {
                r->rendererInit(renderer);
            }
        }

        inputHandlers_.clear();
        for (auto & s: subsystems_) {
            auto x = std::dynamic_pointer_cast<IInputHandler>(s);
            if (x) {
                inputHandlers_.push_back(x);
            }
        }

        hOnMouseButton = mouse_->onMouseButton.AddRaw(this, &Scene::OnMouseButton);
        hOnMousePos = mouse_->onMousePos.AddRaw(this, &Scene::OnMousePos);
        hOnMouseScroll = mouse_->onScroll.AddRaw(this, &Scene::OnMouseScroll);

        hOnKey = keyboard_->onKey.AddRaw(this, &Scene::OnKey);
        hOnChar = keyboard_->onChar.AddRaw(this, &Scene::OnChar);
    }

    void Scene::Shutdown()
    {
        mouse_->onMouseButton.Remove(hOnMouseButton);
        mouse_->onMousePos.Remove(hOnMousePos);
        mouse_->onScroll.Remove(hOnMouseScroll);
        keyboard_->onChar.Remove(hOnChar);
        keyboard_->onKey.Remove(hOnKey);
#if 0
        for (auto * s: subsystemSequences_[Sequences::Shutdown]) {
            s->Shutdown();
        }
#endif
        inputHandlers_.clear();
        for (auto & sequence: subsystemSequences_) {
            sequence.clear();
        }
        mainCamera_.reset();
        //assetLoader.reset();
        subsystems_.clear();
    }

    void Scene::GetRenderables(std::vector<RxEngine::IRenderable *> & renderList)
    {
        renderList.clear();

        for (auto * subsystem: subsystemSequences_[static_cast<uint8_t>(Sequences::Render)]) {
            auto * r = dynamic_cast<RxEngine::IRenderable *>(subsystem);
            if (r) {
                renderList.push_back(r);
            }
        }
    }

    void Scene::GetRenderProviders(std::vector<RxEngine::IRenderProvider *> & renderList)
    {
        renderList.clear();

        for (auto * subsystem: subsystemSequences_[static_cast<uint8_t>(Sequences::Render)]) {
            auto * r = dynamic_cast<RxEngine::IRenderProvider *>(subsystem);
            if (r) {
                renderList.push_back(r);
            }
        }
    }

    void Scene::Update(float delta)
    {
        delta_ = delta;
        const auto fixed_frame_length = 1 / fixedFramesPerSecond_;
        {
            OPTICK_EVENT("Pre Update")
            for (auto * s: subsystemSequences_[static_cast<uint8_t>(Sequences::PreUpdate)]) {
                s->PreUpdate();
            }
        }
        deltaAccumulated_ += delta_;
        if (deltaAccumulated_ > 10) {
            deltaAccumulated_ = 0;
        }
        {
            OPTICK_EVENT("Fixed Update")
            while (deltaAccumulated_ > fixed_frame_length) {
                for (auto * s: subsystemSequences_[static_cast<uint8_t>(Sequences::FixedUpdate)]) {
                    s->FixedUpdate();
                }
                //ecs_.progress(fixed_frame_length);

                deltaAccumulated_ -= fixed_frame_length;
            }
        }
        {
            OPTICK_EVENT("Update")
            for (auto * s: subsystemSequences_[static_cast<uint8_t>(Sequences::Update)]) {
                s->Update(delta_);
            }
        }
        {
            OPTICK_EVENT("Late Update")

            for (auto * s: subsystemSequences_[static_cast<uint8_t>(Sequences::LateUpdate)]) {
                s->LateUpdate();
            }
        }
        {
            OPTICK_EVENT("GUI Update")

            for (auto * s: subsystemSequences_[static_cast<uint8_t>(Sequences::UpdateGui)]) {
                s->UpdateGui();
            }
        }
    }

    void Scene::HidePointer() const
    {
        window_->SetMouseVisible(false);
    }

    void Scene::ShowPointer() const
    {
        window_->SetMouseVisible(true);
    }

    void Scene::addSubsystem(std::shared_ptr<Subsystem> subsys)
    {
        subsys->addedToScene(this, subsystems_);

        inputHandlers_.clear();
        for (auto & s: subsystems_) {
            auto x = std::dynamic_pointer_cast<IInputHandler>(s);
            if (x) {
                inputHandlers_.push_back(x);
            }
        }

        for (auto & subsystem: subsystems_) {
            subsystem->onSubsystemAdded(subsys);
        }
        subsystems_.push_back(subsys);
    }

    void Scene::removeSubsystem(const std::shared_ptr<Subsystem> & subsys)
    {
        for (auto & subsystem: subsystems_) {
            subsystem->onSubsystemRemoved(subsys);
        }

        inputHandlers_.clear();
        for (auto & s: subsystems_) {
            auto x = std::dynamic_pointer_cast<IInputHandler>(s);
            if (x) {
                inputHandlers_.push_back(x);
            }
        }

        std::erase(subsystems_, subsys);

        subsys->removedFromScene();
    }

    void Scene::CreateSubsystemSequence(size_t n)
    {
        std::vector<SubsystemPri> pri;

        pri.resize(subsystems_.size());
        subsystemSequences_[n].resize(subsystems_.size());

        std::ranges::transform(
            subsystems_, pri.begin(), [n](const std::shared_ptr<Subsystem> & s)
            {
                auto v = s->GetCallPriority(static_cast<Sequences>(n));
                auto * sp = s.get();

                return SubsystemPri{sp, v};
            }
        );

        std::ranges::sort(
            pri, [](const SubsystemPri a, const SubsystemPri b)
            {
                return a.pri < b.pri;
            }
        );

        std::ranges::transform(
            pri, subsystemSequences_[n].begin(), [](const SubsystemPri x)
            {
                return x.s;
            }
        );
    }

    void Scene::setSceneCamera(std::shared_ptr<RxEngine::SceneCamera> sceneCamera)
    {
        mainCamera_ = sceneCamera;
    }

    std::shared_ptr<RxEngine::SceneCamera> Scene::getSceneCamera() const
    {
        return mainCamera_;
    }
#if 0
    std::shared_ptr<RXEngine::SceneAssetLoader> Scene::getLoader() const
    {
        return assetLoader;
    }
#endif
    uint32_t Scene::getWidth() const
    {
        return window_->GetWidth();
    }

    uint32_t Scene::getHeight() const
    {
        return window_->GetHeight();
    }

    float Scene::getAspectRatio() const
    {
        return static_cast<float>(getWidth()) / static_cast<float>(getHeight());
    }

    void Scene::resized(int w, int h)
    {
        onResize.Broadcast(w, h);
    }

    void Scene::OnMouseButton(int32_t button, bool pressed, RxEngine::EInputMod mods)
    {
        if (mouseCapturer) {
            mouseCapturer->OnMouseButton(button, pressed, mods);
            return;
        }
        for (auto & ih: inputHandlers_) {
            auto b = ih->OnMouseButton(button, pressed, mods);
            if (b) {
                return;
            }
        }
    }

    void Scene::OnMousePos(float x, float y, RxEngine::EInputMod mods)
    {
        if (mouseCapturer) {
            mouseCapturer->OnMousePos(x, y, mods);
            return;
        }
        for (auto & ih: inputHandlers_) {
            auto b = ih->OnMousePos(x, y, mods);
            if (b) {
                return;
            }
        }
    }

    void Scene::OnMouseScroll(float yscroll, RxEngine::EInputMod mods)
    {
        if (mouseCapturer) {
            mouseCapturer->OnMouseScroll(yscroll, mods);
            return;
        }
        for (auto & ih: inputHandlers_) {
            auto b = ih->OnMouseScroll(yscroll, mods);
            if (b) {
                return;
            }
        }
    }

    void Scene::OnKey(EKey key, EInputAction action, EInputMod mods)
    {
        for (auto & ih: inputHandlers_) {
            auto b = ih->OnKey(key, action, mods);
            if (b) {
                return;
            }
        }
    }

    void Scene::OnChar(char c)
    {
        for (auto & ih: inputHandlers_) {
            auto b = ih->OnChar(c);
            if (b) {
                return;
            }
        }
    }

    MaterialManager * Scene::getMaterialManager() const
    {
        return engine_->getMaterialManager();
    }

    EngineMain * Scene::getEngine() const
    {
        return engine_;
    }

    EntityManager * Scene::getEntityManager() const
    {
        return engine_->getEntityManager();
    }
}
