#pragma once

#include "Modules/Module.h"
#include "RxECS.h"
#include "imgui.h"
#include "DirectXCollision.h"
#include "EngineMain.hpp"
#include "SerialisationData.h"

struct Sector;

namespace RxEngine
{
    namespace Transforms
    {
        struct WorldPosition
        {
            DirectX::XMFLOAT3 position;
        };

        struct YRotation
        {
            float yRotation;
        };

        struct ScalarScale
        {
            float scale;
        };

        struct BoundingSphere
        {
            DirectX::BoundingSphere boundSphere;
        };

        struct BoundingBox
        {
            DirectX::BoundingBox boundBox;
        };
    }

    class TransformsModule: public Module
    {
    public:
        void registerModule() override;
        void enable() override;
        void disable() override;
        void deregisterModule() override;

    private:

        static void worldPositionGui(ecs::EntityHandle e);
        static void yRotationGui(ecs::EntityHandle e);
        static void scalarScaleGui(ecs::EntityHandle e);
    };
};
