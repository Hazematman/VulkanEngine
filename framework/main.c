#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <interface.h>

#define LIBRARY_FILE "libengine.so"

typedef struct
{
    time_t last_mod;
    void *library;
    Interface func;
    PFN_renderer_init renderer_init;
    PFN_renderer_draw renderer_draw;
} LibraryState;

void get_app_info(Interface *func)
{
    AppInfo *app_info = &func->app_info;
    
    app_info->extension_count = MAX_EXTENSIONS;
    
    /* Get the surface extensions required for vulkan */ 
    SDL_bool result = SDL_Vulkan_GetInstanceExtensions(
        func->window, 
        &app_info->extension_count, 
        app_info->enabled_extensions);
        
    if(!result)
    {
        printf("Failed to get extensions from SDL\n%s\n", SDL_GetError());
    }
}

bool create_surface(Interface *func)
{
    bool result = true;
    if(!SDL_Vulkan_CreateSurface(func->window, func->instance, &func->surface))
    {
        result = false;
        printf("Failed to create surface from SDL\n%s\n", SDL_GetError());
    }
    
    return result;
}

void register_engine_functions(LibraryState *lib_state)
{
    lib_state->renderer_init = SDL_LoadFunction(lib_state->library, "renderer_init");
    lib_state->renderer_draw = SDL_LoadFunction(lib_state->library, "renderer_draw");
}

void register_framework_functions(Interface *func)
{
    func->malloc = malloc;
    func->free = free;
    func->realloc = realloc;
    func->printf = printf;
    func->create_surface = create_surface;
    
    /* Register vulkan functions */
    func->vkCreateInstance = vkCreateInstance;
    func->vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    func->vkEnumeratePhysicalDevices = vkEnumeratePhysicalDevices;
    func->vkGetPhysicalDeviceQueueFamilyProperties = vkGetPhysicalDeviceQueueFamilyProperties;
    func->vkGetPhysicalDeviceSurfaceSupportKHR = vkGetPhysicalDeviceSurfaceSupportKHR;
    func->vkCreateDevice = vkCreateDevice;
    func->vkGetDeviceQueue = vkGetDeviceQueue;
    func->vkGetPhysicalDeviceSurfaceFormatsKHR = vkGetPhysicalDeviceSurfaceFormatsKHR;
    func->vkGetPhysicalDeviceSurfacePresentModesKHR = vkGetPhysicalDeviceSurfacePresentModesKHR;
    func->vkGetPhysicalDeviceSurfaceCapabilitiesKHR = vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
    func->vkCreateSwapchainKHR = vkCreateSwapchainKHR;
    func->vkGetSwapchainImagesKHR = vkGetSwapchainImagesKHR;
    func->vkCreateCommandPool = vkCreateCommandPool;
    func->vkAllocateCommandBuffers = vkAllocateCommandBuffers;
    func->vkCreateSemaphore = vkCreateSemaphore;
    func->vkCreateFence = vkCreateFence;
    func->vkWaitForFences = vkWaitForFences;
    func->vkResetFences = vkResetFences;
    func->vkAcquireNextImageKHR = vkAcquireNextImageKHR;
    func->vkBeginCommandBuffer = vkBeginCommandBuffer;
    func->vkEndCommandBuffer = vkEndCommandBuffer;
    func->vkQueueSubmit = vkQueueSubmit;
    func->vkQueuePresentKHR = vkQueuePresentKHR;
    func->vkCmdPipelineBarrier = vkCmdPipelineBarrier;
    func->vkCmdClearColorImage = vkCmdClearColorImage;
}

void reload_library(LibraryState *lib_state)
{
    bool is_loaded = lib_state->library != NULL;
    struct stat lib_stat;
    stat(LIBRARY_FILE, &lib_stat);
    time_t last_mod = lib_stat.st_mtime;
    bool is_newer = lib_state->last_mod < last_mod;
    
    if(!is_loaded || is_newer)
    {
        if(is_loaded)
        {
            printf("Unloading library\n");
            SDL_UnloadObject(lib_state->library);
            lib_state->library = NULL;
        }
        
        lib_state->library = SDL_LoadObject(LIBRARY_FILE);
        
        if(lib_state->library == NULL)
        {
            printf("Failed to load library\n%s\n", SDL_GetError());
            printf("Mode %ld\n", lib_stat.st_size);
        }
        
        lib_state->last_mod = last_mod;
        
        register_engine_functions(lib_state);
    }
}

int init(LibraryState *lib_state) 
{
    Interface *func = &lib_state->func;
    register_framework_functions(&lib_state->func);
    
    
    /* Now create an SDL window */
    func->window = SDL_CreateWindow(
        "Engine",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        DEFAULT_WIDTH, DEFAULT_HEIGHT,
        SDL_WINDOW_VULKAN
    );
    
    /* Grab any app info we need to init the renderer */
    get_app_info(func);
    
    return 0;
}

int main()
{
    bool running = true;
    LibraryState lib_state = {0};
    if(0 != SDL_Init(SDL_INIT_EVERYTHING))
    {
        printf("Error during SDL init\n%s\n", SDL_GetError());
    }
    
    init(&lib_state);
    
    reload_library(&lib_state);
    
    lib_state.renderer_init(&lib_state.func);
    
    SDL_Event e;
    while(running)
    {
        while(SDL_PollEvent(&e))
        {
            if(e.type == SDL_QUIT)
            {
                running = false;
            }
        }
        
        lib_state.renderer_draw(&lib_state.func);
    }
    
    return 0;
}
