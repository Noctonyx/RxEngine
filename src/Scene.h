#pragma once

#include "Delegates.hpp"
#include "Window.hpp"
#include "EngineMain.hpp"

namespace RxEngine
{
#if 0
    class IInputHandler;
    class SceneCamera;
    class Camera;
    class Window;
    class Subsystem;


    struct Stuff1
    {
        int count;
    };

    class Scene
    {
    public:
        explicit Scene(EngineMain * engine, Window * window)
            : window_(window)
            , engine_(engine)
            , mouseCapturer(nullptr)
        {
            window_->onResize.AddRaw(this, &Scene::resized);

            mouse_ = window_->mouse;
            keyboard_ = window_->keyboard;
        }

        RX_NO_COPY_NO_MOVE(Scene)

        virtual ~Scene()
        {
            mouse_.reset();
            keyboard_.reset();

            window_->onResize.RemoveObject(this);
        };

        virtual void Startup(Renderer * renderer);
        virtual void Shutdown();
        virtual void GetRenderables(std::vector<IRenderable *> & renderList);
        virtual void GetRenderProviders(std::vector<IRenderProvider *> & renderList);

        void OnMouseButton(int32_t button, bool pressed, EInputMod mods);
        void OnMousePos(float x, float y, EInputMod mods);
        void OnMouseScroll(float yscroll, EInputMod mods);
        void OnKey(EKey key, EInputAction action, EInputMod mods);
        void OnChar(char c);

        virtual void Update(float delta);

        void HidePointer() const;
        void ShowPointer() const;

        void captureMouse(IInputHandler * capturer)
        {
            if (!mouseCapturer) {
                mouseCapturer = capturer;
            }
        };

        void releaseMouse(IInputHandler * capturer)
        {
            if (mouseCapturer == capturer) {
                mouseCapturer = nullptr;
            }
        }

        void setSceneCamera(std::shared_ptr<SceneCamera> sceneCamera);
        std::shared_ptr<SceneCamera> getSceneCamera() const;
        void addSubsystem(std::shared_ptr<Subsystem> subsys);
        void removeSubsystem(const std::shared_ptr<Subsystem> & subsys);

        uint32_t getWidth() const;
        uint32_t getHeight() const;
        float getAspectRatio() const;

        MaterialManager * getMaterialManager() const;
        EngineMain * getEngine() const;
        EntityManager * getEntityManager() const;

        MulticastDelegate<int, int> onResize;

    protected:
        float fixedFramesPerSecond_ = 60.0f;

        void CreateSubsystemSequence(size_t n);
        void SetFixedFrameRate(float fixedFps) { fixedFramesPerSecond_ = fixedFps; }

        void resized(int w, int h);

        std::vector<std::shared_ptr<Subsystem>> subsystems_;
        std::array<std::vector<Subsystem *>, 9> subsystemSequences_;

        Window * window_;
        std::shared_ptr<SceneCamera> mainCamera_;
        
        float delta_ = 0.f;
        float deltaAccumulated_ = 0.f;

        EngineMain * engine_;

        std::vector<std::shared_ptr<IInputHandler>> inputHandlers_;

        DelegateHandle hOnMouseButton;
        DelegateHandle hOnMousePos;
        DelegateHandle hOnMouseScroll;
        DelegateHandle hOnKey;
        DelegateHandle hOnChar;

        std::shared_ptr<Mouse> mouse_;
        std::shared_ptr<Keyboard> keyboard_;

        IInputHandler * mouseCapturer;
    };
#endif
}
