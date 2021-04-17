#pragma once
#include "Modules/Render.h"
#include "RxECS.h"

namespace RxEngine
{
    struct MaterialPipelineLua
    {
        ecs::entity_t id;
        int x{ 2 };

        MaterialPipelineLua(ecs::entity_t & e) : id(e) {};
#if 0
        float getLineWidth()
        {
            auto e = id.get<Render::MaterialPipeline>();
            return e->lineWidth;
        }

        void setLineWidth(flecs::world * w, float value)
        {
            auto e = id.mut(w->get_world());
            auto m = e.get_mut<Render::MaterialPipeline>();
            m->lineWidth = value;
            e.modified<Render::MaterialPipeline>();
        }
#endif
    };
}
