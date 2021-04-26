#pragma once

#include "Modules/Module.h"
#include "RxECS.h"
#include "DirectXCollision.h"
#include "EngineMain.hpp"

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

        struct XRotation
        {
            float xRotation;
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
        TransformsModule(ecs::World * world, EngineMain * engine)
            : Module(world, engine) {}

        void registerModule() override;
        void deregisterModule() override;

    private:

        static void worldPositionGui(ecs::EntityHandle e);
        static void yRotationGui(ecs::EntityHandle e);
        static void xRotationGui(ecs::EntityHandle e);
        static void scalarScaleGui(ecs::EntityHandle e);
    };
};
