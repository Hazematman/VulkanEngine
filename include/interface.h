#ifndef INTERFACE_H
#define INTERFACE_H
#include <stdbool.h>
#include <stdint.h>
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>

#define MAX_EXTENSIONS 16
#define MAX_DEVICE_COUNT 2
#define MAX_QUEUE_COUNT 4
#define MAX_PRESENT_MODES_COUNT 6
#define MAX_SWAPCHAIN_IMAGES 3
#define MAX_FRAMES 2

#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 600


typedef struct
{
    unsigned int extension_count;
    const char *enabled_extensions[MAX_EXTENSIONS];
} AppInfo;

struct Interface;
typedef struct Interface Interface;

/* Framework exported functions */
typedef void*(*PFN_malloc)(size_t size);
typedef void (*PFN_free)(void *ptr);
typedef void*(*PFN_realloc)(void *ptr, size_t size);
typedef int (*PFN_printf)(const char *str, ...);

typedef bool (*PFN_create_surface)(Interface *func);

struct Interface
{
    /* Platform functions */
    PFN_malloc malloc;
    PFN_free free;
    PFN_realloc realloc;
    PFN_printf printf;
    PFN_create_surface create_surface;
    
    /* Vulkan functions */
    PFN_vkCreateInstance vkCreateInstance;
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
    PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT;
    PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
    PFN_vkCreateDevice vkCreateDevice;
    PFN_vkGetDeviceQueue vkGetDeviceQueue;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
    PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
    PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
    PFN_vkCreateCommandPool vkCreateCommandPool;
    PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
    PFN_vkCreateSemaphore vkCreateSemaphore;
    PFN_vkCreateFence vkCreateFence;
    PFN_vkWaitForFences vkWaitForFences;
    PFN_vkResetFences vkResetFences;
    PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
    PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
    PFN_vkEndCommandBuffer vkEndCommandBuffer;
    PFN_vkQueueSubmit vkQueueSubmit;
    PFN_vkQueuePresentKHR vkQueuePresentKHR;
    PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier;
    PFN_vkCmdClearColorImage vkCmdClearColorImage;
    
    /* Data */
    AppInfo app_info;
    
    /* SDL information */
    SDL_Window *window;
    
    /* Vulkan information */
    VkInstance instance;
    VkDebugReportCallbackEXT debug_callback;
    VkSurfaceKHR surface;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkQueue queue;
    uint32_t queue_family_index;
    uint32_t swapchain_image_count;
    VkSwapchainKHR swapchain;
    VkImage swapchain_images[MAX_SWAPCHAIN_IMAGES];
    VkCommandPool cmd_pool;
    VkCommandBuffer cmd_buffers[MAX_FRAMES];
    VkSemaphore img_avaliable_sem[MAX_FRAMES];
    VkSemaphore render_finished_sem[MAX_FRAMES];
    VkFence frame_fence[MAX_FRAMES];
    uint32_t frame_index;
};

/* Engine exported functions */
typedef void (*PFN_test)(Interface *func);
typedef int (*PFN_renderer_init)(Interface *func);
typedef void (*PFN_renderer_draw)(Interface *func);

#endif
