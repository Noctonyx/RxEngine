//
// Created by shane on 13/02/2021.
//

#ifndef RX_IMGUIPIPELINELAYOUT_H
#define RX_IMGUIPIPELINELAYOUT_H

#include <Vulkan/IPipelineLayout.h>
#include "Vulkan/IPipelineLayout.h"

namespace RxEngine
{
    class ImGuiPipelineLayout : public RxCore::IPipelineLayout
    {
    public:
        vk::DescriptorSetLayout getDescriptorSetlayout(uint32_t set) override;

        std::unordered_map<uint32_t, vk::DescriptorSetLayout> dsLayouts;
        vk::PipelineLayout pipelineLayout;
    };
}
#endif //RX_IMGUIPIPELINELAYOUT_H
