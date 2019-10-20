#ifndef INTERFACE_H
#define INTERFACE_H
#include <stdbool.h>
#include <stdint.h>
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>

#define MAX_EXTENSIONS 16

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
    
    /* Data */
    AppInfo app_info;
    
    /* SDL information */
    SDL_Window *window;
    
    /* Vulkan information */
    VkInstance instance;
    VkDebugReportCallbackEXT debug_callback;
    VkSurfaceKHR surface;
};

/* Engine exported functions */
typedef void(*PFN_test)(Interface *func);
typedef int(*PFN_renderer_init)(Interface *func);

#endif
