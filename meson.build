project('engine', 'c')

engine_files = [
    'engine/renderer/vulkan/renderer_vk.c',
    'engine/renderer/vulkan/renderer_vk_draw.c',
    'engine/util/util_file.c',
]

framework_files = [
    'framework/main.c'
]

sdl2 = dependency('sdl2')
vulkan = dependency('vulkan')

incdir = include_directories('include')

engine_incdir = include_directories('include/engine')

lib = shared_library('engine', engine_files, include_directories : [incdir, engine_incdir])

executable('engine', framework_files, link_with : lib, include_directories : incdir, dependencies : [sdl2, vulkan])

