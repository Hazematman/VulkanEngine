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
    .enabledLayerCount = enabled_layer_count,
    .ppEnabledLayerNames = enabled_layers,
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
    
    if(!error && !init_surface(func))
    {
        error = true;
        func->printf("Failed to create surface\n");
    }
    
    if(!error && !init_device(func))
    {
        error = true;
        func->printf("Failed to create device\n");
    }
    
    if(!error && !init_swapchain(func))
    {
        error = true;
        func->printf("Failed to create swapchain\n");
    }
    
    return error;
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
    
        result = func->vkCreateInstance(&create_info, 0, &func->instance);
    
        if(result == VK_SUCCESS)
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
        else
        {
            func->printf("Failed to create instance\n");
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
        func->vkGetSwapchainImagesKHR(func->device, swapchain, &swapchain_image_count, func->swapchain_images);
        func->swapchain_image_count = swapchain_image_count;
        func->swapchain = swapchain;
    }
    
    return result == VK_SUCCESS;
}
