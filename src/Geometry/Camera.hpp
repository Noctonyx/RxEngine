#pragma once
#include "DirectXMath.h"
#include "DirectXCollision.h"

namespace RxEngine
{
    struct CameraShaderData
    {
        DirectX::XMFLOAT4X4 projection;
        DirectX::XMFLOAT4X4 view;
        DirectX::XMFLOAT3 viewPos;
    };

    class Camera
    {
    public :
        void updateViewMatrix()
        {
            DirectX::XMMATRIX rotM = DirectX::XMMatrixRotationRollPitchYawFromVector(DirectX::XMLoadFloat3(&rotation));
            DirectX::XMMATRIX transM = DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(&basePosition));
            DirectX::XMMATRIX dollyM = DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(&dolly));

            auto iView = dollyM * rotM * transM;
            auto view = DirectX::XMMatrixInverse(nullptr, iView);

            DirectX::XMStoreFloat4x4(&cameraShaderData.view, view);
            DirectX::XMStoreFloat4x4(&iview, iView);

            DirectX::XMStoreFloat3(
                &cameraShaderData.viewPos,
                DirectX::XMVector3TransformCoord(
                    DirectX::XMVectorZero(),
                    iView
                )
            );
            auto proj = DirectX::XMLoadFloat4x4(&cameraShaderData.projection);
            auto viewProj = view * proj;

            DirectX::XMStoreFloat4x4(&projView, viewProj);
            DirectX::XMStoreFloat4x4(&iProjView, DirectX::XMMatrixInverse(nullptr, viewProj));
#if 0
            auto xxx = DirectX::BoundingFrustum(proj);
            xxx.Transform(frustum_, iview);
#endif
            updated = true;
        }

    public:

        Camera() = default;

        DirectX::XMFLOAT3 rotation;
        DirectX::XMFLOAT3 basePosition;
        DirectX::XMFLOAT3 dolly;

        float rotationSpeed = 1.0f;
        float movementSpeed = 1.0f;

        bool updated = false;

        CameraShaderData cameraShaderData{};
        DirectX::XMFLOAT4X4 iProjView{};
        DirectX::XMFLOAT4X4 projView{};
        DirectX::XMFLOAT4X4 iview;

        float fov_{};
        float zNear_{}, zFar_{};

        [[nodiscard]] float getNearClip() const
        {
            return zNear_;
        }

        [[nodiscard]] float getFarClip() const
        {
            return zFar_;
        }

        void setPerspective(
            const float fov,
            const float aspect,
            const float zNear,
            const float zFar
        )
        {
            this->fov_ = fov;
            this->zNear_ = zNear;
            this->zFar_ = zFar;

            DirectX::XMStoreFloat4x4(
                &cameraShaderData.projection, DirectX::XMMatrixPerspectiveFovRH(
                    DirectX::XMConvertToRadians(fov_),
                    aspect,
                    zNear_,
                    zFar_
                )
            );
            updateViewMatrix();
        }

        void updateAspectRatio(const float aspect)
        {
            DirectX::XMStoreFloat4x4(
                &cameraShaderData.projection,
                DirectX::XMMatrixPerspectiveFovRH(
                    DirectX::XMConvertToRadians(fov_),
                    aspect,
                    zNear_,
                    zFar_)
            );
            updateViewMatrix();
        }

        void setPosition(DirectX::XMFLOAT3 position)
        {
            this->basePosition = position;
            updateViewMatrix();
        }

        void setRotation(DirectX::XMVECTOR rot)
        {
            DirectX::XMStoreFloat3(&this->rotation, rot);
            updateViewMatrix();
        }
#if 0
        void rotate(const DirectX::XMFLOAT3 delta)
        {
            DirectX::XMStoreFloat3()
            this->rotation += delta;
            updateViewMatrix();
        }
#endif
        void setTranslation(const DirectX::XMFLOAT3 translation)
        {
            this->basePosition = translation;
            updateViewMatrix();
        };

        void translate(const DirectX::XMVECTOR delta)
        {
            auto r = DirectX::XMVectorAdd(DirectX::XMLoadFloat3(&this->basePosition), delta);
            DirectX::XMStoreFloat3(&this->basePosition, r);
            //this->basePosition = this->basePosition + delta;
            updateViewMatrix();
        }

        void setDolly(const DirectX::XMVECTOR d)
        {
            DirectX::XMStoreFloat3(&dolly, d);
            //dolly = d;
            updateViewMatrix();
        }

        void setRotationSpeed(const float rotSpeed)
        {
            this->rotationSpeed = rotSpeed;
        }

        void setMovementSpeed(const float moveSpeed)
        {
            this->movementSpeed = moveSpeed;
        }

        [[nodiscard]] DirectX::XMVECTOR getForward() const
        {
            auto v = DirectX::XMVectorSet(0.f, 0.f, -1.f, 0.f);
            v = DirectX::XMVector3TransformNormal(
                    v,
                    DirectX::XMLoadFloat4x4(&iview));
            v = DirectX::XMVector3Normalize(v);
            return v;
        }

        [[nodiscard]] DirectX::XMVECTOR getUp() const
        {
            auto v = DirectX::XMVectorSet(0.f, 1.f, 0.f, 0.f);
            v = DirectX::XMVector3TransformNormal(
                    v,
                    DirectX::XMLoadFloat4x4(&iview));
            v = DirectX::XMVector3Normalize(v);
            return v;
        }

        [[nodiscard]] DirectX::XMVECTOR getRight() const
        {
            return DirectX::XMVector3Normalize(
                DirectX::XMVector3TransformNormal(
                    DirectX::XMVectorSet(1.f, 0.f, 0.f, 0.f),
                    DirectX::XMLoadFloat4x4(&iview)));
        }
    };
}
