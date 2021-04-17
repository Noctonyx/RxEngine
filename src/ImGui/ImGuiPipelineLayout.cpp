//
// Created by shane on 13/02/2021.
//

#include "ImGuiPipelineLayout.h"

namespace RxEngine
{
    vk::DescriptorSetLayout ImGuiPipelineLayout::getDescriptorSetlayout(uint32_t set)
    {
        return dsLayouts[set];
    }
}