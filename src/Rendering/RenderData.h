//
// Created by shane on 24/01/2021.
//

#ifndef RX_RENDERDATA_H
#define RX_RENDERDATA_H

#include <memory>
#include <optional>
#include "Vulkan/Vulk.hpp"

namespace RxCore
{
    class RenderPass;
    class SecondaryCommandBuffer;
    class FrameBuffer;
}

namespace RxEngine
{
    enum ERenderSequence
    {
        RenderSequenceShadowPass,
        RenderSequenceOpaque,
        RenderSequenceTransparent,
        RenderSequencePost,
        RenderSequenceUi,

        RenderSequenceCascade1 = 1000,
        RenderSequenceCascade2,
        RenderSequenceCascade3,
        RenderSequenceCascade4,
        RenderSequenceCascade5,
        RenderSequenceCascade6,
    };

    struct RenderStage
    {
        vk::RenderPass renderPass;
        uint32_t subPass;
    };

    struct FrameBufferDetails
    {
        vk::Extent2D extent;
        std::shared_ptr<RxCore::FrameBuffer> fb;
    };

    //typedef std::pair<RenderPass::ptr, uint32_t> RenderStage;
    typedef std::optional<std::shared_ptr<RxCore::SecondaryCommandBuffer>> RenderResponse;
}

#endif //RX_RENDERDATA_H
