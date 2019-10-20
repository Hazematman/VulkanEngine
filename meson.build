project('engine', 'c')

engine_files = [
    'engine/renderer/vulkan/renderer_vk.c'
]

framework_files = [
    'framework/main.c'
]

sdl2 = dependency('sdl2')
vulkan = dependency('vulkan')

incdir = include_directories('include')

lib = shared_library('engine', engine_files, include_directories : incdir)

executable('engine', framework_files, link_with : lib, include_directories : incdir, dependencies : [sdl2, vulkan])

