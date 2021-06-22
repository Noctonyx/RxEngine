//
// Created by shane on 29/12/2020.
//

#ifndef RX_COMMANDPOOL_HPP
#define RX_COMMANDPOOL_HPP

#include <memory>
#include "Vulk.hpp"

//#include "Rendering/Renderer.hpp"

namespace RxCore
{
    class CommandBuffer;
    class SecondaryCommandBuffer;
    class PrimaryCommandBuffer;
    class TransferCommandBuffer;
    class Device;

    class RenderPass;

    class CommandPool : public std::enable_shared_from_this<CommandPool>
    {
    public:
        typedef std::shared_ptr<CommandBuffer> ptr;

        CommandPool(Device * device, VkCommandPool handle, uint32_t qf);
        //CommandPool(uint32_t queueFamily);
        ~CommandPool();

        std::shared_ptr<PrimaryCommandBuffer> GetPrimaryCommandBuffer();
        std::shared_ptr<TransferCommandBuffer> createTransferCommandBuffer();
        std::shared_ptr<SecondaryCommandBuffer> GetSecondaryCommandBuffer();

        VkCommandPool GetHandle() const { return handle; }

    private:
        VkCommandPool handle;
        Device * device_;
        //uint32_t queuefamily_;
    };
} // namespace RXCore
#endif // RX_COMMANDPOOL_HPP
