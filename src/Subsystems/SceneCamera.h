#pragma once

#include <Subsystem.h>
#include "Geometry/Camera.hpp"
#include "Vulkan/DescriptorSet.hpp"

namespace RxEngine
{
    class Scene;
    class Camera;
    class RenderCamera;

    class SceneCamera : public Subsystem //, public IRenderProvider
    {
    public:
        explicit SceneCamera();

        void UpdateGui() override;

        [[nodiscard]] std::shared_ptr<Camera> GetCamera() const
        {
            return camera_;
        }

        void addedToScene(
            Scene * scene,
            std::vector<std::shared_ptr<Subsystem>> & subsystems) override;

    private:
        std::shared_ptr<Camera> camera_;
    };
}
