#include <stdbool.h>
#include <stdint.h>
#include "interface.h"
#include "renderer_int.h"

void renderer_draw(Interface *func)
{
    uint32_t index = func->frame_index;
    uint32_t image_index;
    VkCommandBufferBeginInfo begin_info = {0};
    VkSubmitInfo submit_info = {0};
    VkPresentInfoKHR present_info = {0};
    VkClearColorValue clear_color = {{1.0f, 0.0f, 0.0f, 1.0f}};
    VkImageSubresourceRange sub_resource_range = {0};
    VkImageMemoryBarrier optimal_clear = {0};
    VkImageMemoryBarrier optimal_present = {0};
    
    
    func->vkWaitForFences(func->device, 1, &func->frame_fence[index], VK_TRUE, UINT64_MAX);
    func->vkResetFences(func->device, 1, &func->frame_fence[index]);
    
    func->vkAcquireNextImageKHR(
        func->device, func->swapchain, UINT64_MAX, func->img_avaliable_sem[index],
        VK_NULL_HANDLE, &image_index);
        
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    func->vkBeginCommandBuffer(func->cmd_buffers[index], &begin_info);
    
    sub_resource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    sub_resource_range.baseMipLevel = 0;
    sub_resource_range.levelCount = VK_REMAINING_MIP_LEVELS;
    sub_resource_range.baseArrayLayer = 0;
    sub_resource_range.layerCount = VK_REMAINING_ARRAY_LAYERS;
    
    optimal_clear.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    optimal_clear.srcAccessMask = 0;
    optimal_clear.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    optimal_clear.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    optimal_clear.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    optimal_clear.srcQueueFamilyIndex = func->queue_family_index;
    optimal_clear.dstQueueFamilyIndex = func->queue_family_index;
    optimal_clear.image = func->swapchain_images[image_index];
    optimal_clear.subresourceRange = sub_resource_range;
    
    func->vkCmdPipelineBarrier(
        func->cmd_buffers[index], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, NULL, 0, NULL, 1,
        &optimal_clear);
    
    func->vkCmdClearColorImage(
        func->cmd_buffers[index], func->swapchain_images[index],
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        &clear_color, 1, &sub_resource_range);
    
    optimal_present.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    optimal_present.srcAccessMask = 0;
    optimal_present.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    optimal_present.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    optimal_present.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    optimal_present.srcQueueFamilyIndex = func->queue_family_index;
    optimal_present.dstQueueFamilyIndex = func->queue_family_index;
    optimal_present.image = func->swapchain_images[image_index];
    optimal_present.subresourceRange = sub_resource_range;
    
    func->vkCmdPipelineBarrier(
        func->cmd_buffers[index], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, NULL, 0, NULL, 1,
        &optimal_present);
    
    func->vkEndCommandBuffer(func->cmd_buffers[index]);
    
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &func->img_avaliable_sem[index];
    submit_info.pWaitDstStageMask = (VkPipelineStageFlags[]) { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &func->cmd_buffers[index];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &func->render_finished_sem[index];
    
    func->vkQueueSubmit(func->queue, 1, &submit_info, func->frame_fence[index]);
    
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &func->render_finished_sem[index];
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &func->swapchain;
    present_info.pImageIndices = &image_index;
    
    func->vkQueuePresentKHR(func->queue, &present_info);
    
    func->frame_index = (func->frame_index + 1) % MAX_FRAMES;
}
