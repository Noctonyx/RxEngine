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

        struct LocalRotation
        {
            DirectX::XMFLOAT3 rotation;
        };
#if 0
        struct YRotation
        {
            float yRotation;
        };

        struct XRotation
        {
            float xRotation;
        };
#endif
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

        //static void worldPositionGui(void * ptr);
        //static void yRotationGui(void* ptr);
        //static void xRotationGui(void* ptr);
        //static void scalarScaleGui(void* ptr);
    };
};
