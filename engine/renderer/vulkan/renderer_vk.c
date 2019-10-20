#include <stdbool.h>
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
