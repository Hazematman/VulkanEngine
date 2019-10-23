#include <stdbool.h>
#include <stdint.h>
#include "interface.h"
#include "renderer_int.h"

static const VkApplicationInfo app_info = 
{
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = "Engine App",
    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    .pEngineName = "Haze Engine",
    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
    .apiVersion = VK_API_VERSION_1_0,
};

static const char *enabled_layers[] = {"VK_LAYER_LUNARG_standard_validation"};
static const int enabled_layer_count = sizeof(enabled_layers)/sizeof(char*);

static VkInstanceCreateInfo create_info =
{
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &app_info,
};

/* Call back for debug function */
static VKAPI_ATTR VkBool32 VKAPI_CALL prv_report_function(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT obj_type,
    uint64_t obj,
    size_t location,
    int32_t code,
    const char* layer_prefix,
    const char* msg,
    void* userData)
{
    printf("VULKAN VALIDATION: %s\n", msg);
    return VK_FALSE;
}

static VkDebugReportCallbackCreateInfoEXT debug_callback_create_info = {
    .sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
    .flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT,
    .pfnCallback = prv_report_function,
};

int renderer_init(Interface *func)
{
    bool error = false;
    
    if(!init_vulkan(func))
    {
        error = true;
        func->printf("Failed to create instance\n");
    }
    else if(!init_surface(func))
    {
        error = true;
        func->printf("Failed to create surface\n");
    }
    else if(!init_device(func))
    {
        error = true;
        func->printf("Failed to create device\n");
    }
    else if(!init_swapchain(func))
    {
        error = true;
        func->printf("Failed to create swapchain\n");
    }
    else if(!init_render(func))
    {
        error = true;
        func->printf("Failed to create render construct\n");
    }
    else if(!init_render_pass(func))
    {
        error = true;
        func->printf("Failed to create render pass\n");
    }
    else if(!init_framebuffers(func))
    {
        error = true;
        func->printf("Failed to create framebuffers\n");
    }
    else if(!init_shaders(func))
    {
        error = true;
        func->printf("Failed to create shaders\n");
    }
    else if(!init_pipeline(func))
    {
        error = true;
        func->printf("Failed to create pipeline\n");
    }
    
    
    return !error;
}

bool init_vulkan(Interface *func)
{
    VkResult result = VK_ERROR_INITIALIZATION_FAILED;
    
    if(func->app_info.extension_count < MAX_EXTENSIONS)
    {
        func->app_info.enabled_extensions[func->app_info.extension_count] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
        func->app_info.extension_count += 1;
        
        create_info.enabledExtensionCount = func->app_info.extension_count;
        create_info.ppEnabledExtensionNames = func->app_info.enabled_extensions;
        
        if(func->app_info.debug)
        {
            create_info.ppEnabledLayerNames = enabled_layers;
            create_info.enabledLayerCount = enabled_layer_count;
        }
    
        result = func->vkCreateInstance(&create_info, 0, &func->instance);
    
        if(result == VK_SUCCESS)
        {
            if(func->app_info.debug)
            {
                result = VK_ERROR_INITIALIZATION_FAILED;
                /* Since debug reporting is an extension
                   we need to use getProcAddr to actually
                   find the function pointer */
                func->vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)func->vkGetInstanceProcAddr(func->instance, "vkCreateDebugReportCallbackEXT");
                
                if(func->vkCreateDebugReportCallbackEXT)
                {
                    result = func->vkCreateDebugReportCallbackEXT(func->instance, &debug_callback_create_info, 0, &func->debug_callback);
                }
                else
                {
                    func->printf("Failed to find debug report callback\n");
                }
            }
        }
        else
        {
            func->printf("Failed to create instance with error %d\n", result);
        }
    }
    else
    {
        func->printf("Max number of extensions reached\n");
    }
    
    return result == VK_SUCCESS;
}

bool init_surface(Interface *func)
{
    return func->create_surface(func);
}

bool init_device(Interface *func)
{
    VkResult result = VK_ERROR_INITIALIZATION_FAILED;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    uint32_t queue_family_index;
    uint32_t physical_device_count = MAX_DEVICE_COUNT;
    VkPhysicalDevice device_handles[MAX_DEVICE_COUNT];
    uint32_t queue_family_count;
    VkQueueFamilyProperties queue_family_properties[MAX_QUEUE_COUNT];
    VkBool32 supports_present;
    VkQueue queue;
    VkDevice device = VK_NULL_HANDLE;
    VkDeviceCreateInfo device_create_info = {0};
    VkDeviceQueueCreateInfo queue_create_info = {0};
    
    func->vkEnumeratePhysicalDevices(func->instance, &physical_device_count, device_handles);
    
    for(uint32_t i = 0; i < physical_device_count; i++)
    {
        queue_family_count = MAX_QUEUE_COUNT;
        func->vkGetPhysicalDeviceQueueFamilyProperties(
            device_handles[i], &queue_family_count, queue_family_properties);
        for(uint32_t j = 0; j < queue_family_count; j++)
        {
            supports_present = VK_FALSE;
            func->vkGetPhysicalDeviceSurfaceSupportKHR(device_handles[i], j, func->surface, &supports_present);
            
            if(supports_present && (queue_family_properties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT))
            {
                queue_family_index = j;
                physical_device = device_handles[i];
                result = VK_SUCCESS;
                break;
            }
        }
        
        if(physical_device)
        {
            break;
        }
    }
    
    if(result == VK_SUCCESS)
    {
        device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_create_info.queueCreateInfoCount = 1;
        device_create_info.pQueueCreateInfos = &queue_create_info;
        device_create_info.enabledExtensionCount = 1;
        device_create_info.ppEnabledExtensionNames = (const char* const[]) {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queue_family_index;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = (const float[]) {1.0f};
        
        result = func->vkCreateDevice(physical_device, &device_create_info, NULL, &device);
        
        if(result != VK_SUCCESS)
        {
            func->printf("Failed to create device\n");
        }
        else
        {
            func->vkGetDeviceQueue(device, queue_family_index, 0, &queue);
        }
    }
    else
    {
        func->printf("Failed to find a physical device\n");
    }
    
    if(result == VK_SUCCESS)
    {
        /* If we know everything completed successfully then
         * assign the create device to the interface         */

        func->physical_device = physical_device;
        func->device = device;
        func->queue = queue;
        func->queue_family_index = queue_family_index;
    }
    
    return result == VK_SUCCESS;
}

#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define CLAMP(x,y,z) (MIN((z), MAX((x), (y))))

bool init_swapchain(Interface *func)
{
    VkResult result = VK_ERROR_INITIALIZATION_FAILED;
    uint32_t format_count = 1;
    uint32_t swapchain_image_count = 2;
    uint32_t present_mode_count = MAX_PRESENT_MODES_COUNT;
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    VkPresentModeKHR present_modes[MAX_PRESENT_MODES_COUNT];
    VkSurfaceCapabilitiesKHR surface_capabilities;
    VkSwapchainKHR swapchain;
    VkExtent2D swapchain_extent;
    VkSurfaceFormatKHR surface_format;
    VkSwapchainCreateInfoKHR swapchain_create_info = {0};
    
    
    func->vkGetPhysicalDeviceSurfaceFormatsKHR(
        func->physical_device, func->surface, &format_count, &surface_format);
        
    if(surface_format.format == VK_FORMAT_UNDEFINED)
    {
        surface_format.format = VK_FORMAT_B8G8R8A8_UNORM;
    }
    
    func->vkGetPhysicalDeviceSurfacePresentModesKHR(
        func->physical_device, func->surface, &present_mode_count, present_modes);
        
    for(uint32_t i = 0; i < present_mode_count; i++)
    {
        switch(present_modes[i])
        {
            case VK_PRESENT_MODE_MAILBOX_KHR:
                present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
                swapchain_image_count = 3;
                break;
            default:
                break;
        }
    }
    
    func->vkGetPhysicalDeviceSurfaceCapabilitiesKHR(func->physical_device, func->surface, &surface_capabilities);
    
    swapchain_extent = surface_capabilities.currentExtent;
    if(swapchain_extent.width == UINT32_MAX)
    {
        swapchain_extent.width = CLAMP(
            DEFAULT_WIDTH, 
            surface_capabilities.minImageExtent.width,
            surface_capabilities.maxImageExtent.width);
        swapchain_extent.height = CLAMP(
            DEFAULT_HEIGHT, 
            surface_capabilities.minImageExtent.height,
            surface_capabilities.maxImageExtent.height);
    }
    
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface = func->surface;
    swapchain_create_info.minImageCount = swapchain_image_count;
    swapchain_create_info.imageFormat = surface_format.format;
    swapchain_create_info.imageColorSpace = surface_format.colorSpace;
    swapchain_create_info.imageExtent = swapchain_extent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.preTransform = surface_capabilities.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = present_mode;
    swapchain_create_info.clipped = VK_TRUE;
    
    result = func->vkCreateSwapchainKHR(func->device, &swapchain_create_info, 0, &swapchain);
    
    if(result != VK_SUCCESS)
    {
        func->printf("Failed to create swapchain\n");
    }
    else
    {
        func->vkGetSwapchainImagesKHR(func->device, swapchain, &swapchain_image_count, NULL);
        if(swapchain_image_count >= MAX_SWAPCHAIN_IMAGES)
        {
            result = VK_ERROR_INITIALIZATION_FAILED;
            func->printf("Maximum number of swapchain images reached\n");
            func->printf("TODO fix this\n");
        }
        else
        {
            func->vkGetSwapchainImagesKHR(func->device, swapchain, &swapchain_image_count, func->swapchain_images);
            func->swapchain_image_count = swapchain_image_count;
            func->swapchain = swapchain;
            func->surface_format = surface_format;
            func->swapchain_extent = swapchain_extent;
        }
    }
    
    return result == VK_SUCCESS;
}

bool init_render(Interface *func)
{
    VkCommandPoolCreateInfo cmd_pool_create_info = {0};
    VkCommandBufferAllocateInfo cmd_buffer_alloc_info = {0};
    VkSemaphoreCreateInfo sem_create_info = {0};
    VkFenceCreateInfo fence_create_info = {0};
    
    cmd_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmd_pool_create_info.queueFamilyIndex = func->queue_family_index;
    
    func->vkCreateCommandPool(func->device, &cmd_pool_create_info, 0, &func->cmd_pool);
    
    cmd_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buffer_alloc_info.commandPool = func->cmd_pool;
    cmd_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_buffer_alloc_info.commandBufferCount = MAX_FRAMES;
    
    func->vkAllocateCommandBuffers(func->device, &cmd_buffer_alloc_info, func->cmd_buffers);
    
    sem_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for(size_t i = 0; i < MAX_FRAMES; i++)
    {
        func->vkCreateSemaphore(func->device, &sem_create_info, 0, &func->img_avaliable_sem[i]);
        func->vkCreateSemaphore(func->device, &sem_create_info, 0, &func->render_finished_sem[i]);
        func->vkCreateFence(func->device, &fence_create_info, 0, &func->frame_fence[i]);
    }
    
    func->frame_index = 0;
    
    return true;
}

bool init_render_pass(Interface *func)
{
    VkResult result = VK_ERROR_INITIALIZATION_FAILED;
    VkRenderPassCreateInfo render_pass_create_info = {0};
    VkAttachmentDescription attachment_desc = {0};
    VkSubpassDescription subpass_desc = {0};
    VkSubpassDependency subpass_depend = {0};
    VkAttachmentReference attachment_ref = {0};
    
    attachment_desc.format = func->surface_format.format;
    attachment_desc.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment_desc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_desc.colorAttachmentCount = 1;
    subpass_desc.pColorAttachments = &attachment_ref;
    
    attachment_ref.attachment = 0;
    attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    subpass_depend.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_depend.dstSubpass = 0;
    subpass_depend.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_depend.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_depend.srcAccessMask = 0;
    subpass_depend.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpass_depend.dependencyFlags = 0;
    
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    render_pass_create_info.attachmentCount = 1,
    render_pass_create_info.pAttachments = &attachment_desc;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass_desc;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &subpass_depend;
    
    result = func->vkCreateRenderPass(func->device, &render_pass_create_info, 0, &func->render_pass);
    
    return result == VK_SUCCESS;
}

bool init_framebuffers(Interface *func)
{
    VkResult result = VK_ERROR_INITIALIZATION_FAILED;
    VkImageViewCreateInfo image_view_create_info = {0};
    VkFramebufferCreateInfo framebuffer_create_info = {0};
    
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = func->surface_format.format;    
    image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 1;
    
    framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_create_info.renderPass = func->render_pass;
    framebuffer_create_info.attachmentCount = 1;
    framebuffer_create_info.width = func->swapchain_extent.width;
    framebuffer_create_info.height = func->swapchain_extent.height;
    framebuffer_create_info.layers = 1;
    
    
    for(uint32_t i = 0; i < func->swapchain_image_count; i++)
    {
        image_view_create_info.image = func->swapchain_images[i];
        result = func->vkCreateImageView(func->device, &image_view_create_info, 0, &func->swapchain_image_views[i]);
    
        if(result != VK_SUCCESS)
        {
            func->printf("Failed to create image view %d\n", result);
        }
        else
        {
            framebuffer_create_info.pAttachments = &func->swapchain_image_views[i];
            result = func->vkCreateFramebuffer(func->device, &framebuffer_create_info, 0, &func->framebuffers[i]);
        }

        if(result != VK_SUCCESS)
        {
            break;
        }
    }
    
    return result == VK_SUCCESS;
}

bool init_shaders(Interface *func)
{
    VkResult result = VK_ERROR_INITIALIZATION_FAILED;
    VkShaderModuleCreateInfo shader_create_info = {0}; 
    void *vert_shader_data = NULL;
    uint32_t vert_shader_size = 0;
    void *frag_shader_data = NULL;
    uint32_t frag_shader_size = 0;
    
    util_load_whole_file("vert.spirv", &vert_shader_data, &vert_shader_size);
    util_load_whole_file("frag.spirv", &frag_shader_data, &frag_shader_size);
    
    if(!vert_shader_data || !frag_shader_data)
    {
        func->printf("Failed to load vertex or fragment shader\n");
    }
    else
    {
        shader_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shader_create_info.codeSize = vert_shader_size;
        shader_create_info.pCode = vert_shader_data;
        
        result = func->vkCreateShaderModule(func->device, &shader_create_info, 0, &func->vert_shader);
        
        if(result != VK_SUCCESS)
        {
            func->printf("Failed to create vert shader module\n");
        }
        else
        {
            shader_create_info.codeSize = frag_shader_size;
            shader_create_info.pCode = frag_shader_data;
            
            result = func->vkCreateShaderModule(func->device, &shader_create_info, 0, &func->frag_shader);
        }
    }
    
    return result == VK_SUCCESS;
}

bool init_pipeline(Interface *func)
{
    VkResult result = VK_ERROR_INITIALIZATION_FAILED;
    VkPipelineShaderStageCreateInfo shader_stages[2] = {0};
    VkPipelineVertexInputStateCreateInfo vertex_input_state = {0};
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {0};
    VkPipelineRasterizationStateCreateInfo rasteriation_state = {0};
    VkPipelineViewportStateCreateInfo viewport_state = {0};
    VkPipelineMultisampleStateCreateInfo multisample_state = {0};
    VkPipelineColorBlendStateCreateInfo color_blend_state = {0};
    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {0};
    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {0};
    VkGraphicsPipelineCreateInfo pipeline_create_info = {0};
    VkPipelineColorBlendAttachmentState color_blend_attachment_state[1] = {0};
    VkDynamicState dynamic_states[2] = {0};
    
    shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stages[0].module = func->vert_shader;
    shader_stages[0].pName = "main";
    
    shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stages[1].module = func->frag_shader;
    shader_stages[1].pName = "main";
    
    vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    
    input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state.primitiveRestartEnable = VK_FALSE;
    
    rasteriation_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasteriation_state.depthClampEnable = VK_FALSE;
    rasteriation_state.rasterizerDiscardEnable = VK_FALSE;
    rasteriation_state.polygonMode = VK_POLYGON_MODE_FILL;
    rasteriation_state.cullMode = VK_CULL_MODE_BACK_BIT;
    rasteriation_state.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasteriation_state.depthBiasEnable = VK_FALSE;
    rasteriation_state.depthBiasConstantFactor = 0.0f;
    rasteriation_state.depthBiasClamp = 0.0f;
    rasteriation_state.depthBiasSlopeFactor = 0.0f;
    rasteriation_state.lineWidth = 1.0f;
    
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = 0;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = 0;
    
    multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_state.sampleShadingEnable = VK_FALSE;
    multisample_state.minSampleShading = 1.0f;
    multisample_state.pSampleMask = 0;
    multisample_state.alphaToCoverageEnable = VK_FALSE;
    multisample_state.alphaToOneEnable = VK_FALSE;
    
    color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state.logicOpEnable = VK_FALSE;
    color_blend_state.attachmentCount = 1;
    color_blend_state.pAttachments = color_blend_attachment_state;
    
    color_blend_attachment_state[0].blendEnable = VK_FALSE;
    color_blend_attachment_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state[0].colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state[0].alphaBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    
    dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_create_info.dynamicStateCount = 2;
    dynamic_state_create_info.pDynamicStates = dynamic_states;
    
    dynamic_states[0] = VK_DYNAMIC_STATE_VIEWPORT;
    dynamic_states[1] = VK_DYNAMIC_STATE_SCISSOR;
    
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    
    result = func->vkCreatePipelineLayout(func->device, &pipeline_layout_create_info, 0, &func->pipeline_layout);
    
    if(result != VK_SUCCESS)
    {
        func->printf("Failed to create pipeline layout\n");
    }
    else
    {
    
        pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_create_info.stageCount = 2;
        pipeline_create_info.pStages = shader_stages;
        pipeline_create_info.pVertexInputState = &vertex_input_state;
        pipeline_create_info.pInputAssemblyState = &input_assembly_state;
        pipeline_create_info.pViewportState = &viewport_state;
        pipeline_create_info.pRasterizationState = &rasteriation_state;
        pipeline_create_info.pMultisampleState = &multisample_state;
        pipeline_create_info.pColorBlendState = &color_blend_state;
        pipeline_create_info.pDynamicState = &dynamic_state_create_info;
        pipeline_create_info.layout = func->pipeline_layout;
        pipeline_create_info.renderPass = func->render_pass;
        
        result = func->vkCreateGraphicsPipelines(
            func->device, VK_NULL_HANDLE, 1, &pipeline_create_info, 0, &func->pipeline);
    }
    
    return result == VK_SUCCESS;
}
