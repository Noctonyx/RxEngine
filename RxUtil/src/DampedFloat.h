#pragma once
//#include "glm/common.hpp"
#include <cmath>

namespace RxUtil
{
    //template<typename T>
    struct DampedFloat
    {
        float value;
        float acceleration;
        float velocity;
        float maxVelocity;
        float destination;

        DampedFloat(float acceleration, float maxVelocity)
            : value(0)
            , acceleration(acceleration)
            , velocity(0.0f)
            , maxVelocity(maxVelocity)
            , destination(0) { }

        void setDestination(float dest)
        {
            destination = dest;
            velocity = 0.f;
        }

        void update(float deltaTime)
        {
            if (destination < value) {
                velocity -= acceleration * deltaTime;
                if (velocity < -maxVelocity) {
                    velocity = -maxVelocity;
                }
            } else {
                velocity += acceleration * deltaTime;
                if (velocity > maxVelocity) {
                    velocity = maxVelocity;
                }
            }
            if (std::abs(value - destination) < std::abs(velocity * deltaTime)) {
                value = destination;
                velocity = 0;
                return;
            }

            value += velocity * deltaTime;
        }

        float getValue() const
        {
            return value;
        }

        void setValue(float v)
        {
            value = v;
        }
    };
}
