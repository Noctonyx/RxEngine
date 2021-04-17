#ifndef STATS_HPP
#define STATS_HPP

#include <RXCore.h>
#include "Subsystem.h"
#include "Rendering/Renderer.hpp"

namespace RxEngine
{
    class Stats : public Subsystem, public IRenderable
    {
    public:
        explicit Stats()
            : delta_(0.f)
            , fps_(0.f)
            , renderer_(nullptr) {}

        void UpdateGui() override;

        void Update(float delta) override;

        void rendererInit(Renderer * renderer) override;
        std::vector<RenderEntity> getRenderEntities() override;
    private:
        float delta_;
        float fps_;
        const Renderer * renderer_;
        std::deque<float> fpsHistory_;
        std::deque<float> gpuHistory_;
    };
} // namespace RXCore

#endif
