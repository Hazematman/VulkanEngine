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
    VkClearValue clear_value = {.color = {{ 0.0f, 0.1f, 0.2f, 1.0f }}};
    VkRenderPassBeginInfo renderpass_begin = {0};
    VkViewport viewport = {0.0f, 0.0f, func->swapchain_extent.width, func->swapchain_extent.height, 0.0f, 1.0f};
    VkRect2D scissor = {{0, 0}, func->swapchain_extent};
    
    func->vkWaitForFences(func->device, 1, &func->frame_fence[index], VK_TRUE, UINT64_MAX);
    func->vkResetFences(func->device, 1, &func->frame_fence[index]);
    
    func->vkAcquireNextImageKHR(
        func->device, func->swapchain, UINT64_MAX, func->img_avaliable_sem[index],
        VK_NULL_HANDLE, &image_index);
        
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    func->vkBeginCommandBuffer(func->cmd_buffers[index], &begin_info);
    
    renderpass_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderpass_begin.renderPass = func->render_pass;
    renderpass_begin.framebuffer = func->framebuffers[image_index];
    renderpass_begin.clearValueCount = 1;
    renderpass_begin.pClearValues = &clear_value;
    renderpass_begin.renderArea.offset = (VkOffset2D) { .x = 0,.y = 0 };
    renderpass_begin.renderArea.extent = func->swapchain_extent;
    
    func->vkCmdBeginRenderPass(func->cmd_buffers[index], &renderpass_begin, VK_SUBPASS_CONTENTS_INLINE);
    func->vkCmdBindPipeline(func->cmd_buffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, func->pipeline);
    func->vkCmdSetViewport(func->cmd_buffers[index], 0, 1, &viewport);
    func->vkCmdSetScissor(func->cmd_buffers[index], 0, 1, &scissor);
    func->vkCmdDraw(func->cmd_buffers[index], 3, 1, 0, 0);
    
    func->vkCmdEndRenderPass(func->cmd_buffers[index]);
    
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
    
    func->frame_index = (func->frame_index + 1) % func->swapchain_image_count;
}
