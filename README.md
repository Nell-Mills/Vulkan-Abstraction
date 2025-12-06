# Vulkan Abstraction

For using Vulkan 1.3 and SDL3 in my projects.  
For debug, define VKA_DEBUG when compiling.

## Functionality implemented:

- Physical device selection
- Window creation (SDL3)
- Creation of the following:
    - Device
    - Graphics and present queues
    - Command pool, buffers and fences
    - Semaphores
    - Swapchain + images/image views
    - Debug messenger (only when VKA_DEBUG is defined)

## Functionality in progress:

- Pipelines
- Shader modules

## Compilation:

Depends on:

- [Config](https://github.com/Nell-Mills/Config) - expects <NM-Config/Config.h\>
- Volk (included)
- SDL3

Add "Include" to search path: "-I Include"  
Link SDL3: -lSDL3

## Usage:

Generally follow this pattern:

- For the main Vulkan base struct (vka\_vulkan\_t), populate its .config member (optional), then use vka\_setup\_vulkan() and vka\_shutdown\_vulkan().

- For other structs (vka\_X\_t), there will be vka\_create\_X and vka\_destroy\_X functions, and a .config member for you to populate (again, optional).

Structs available:

- vka\_vulkan\_t: Device, queues, command buffers, semaphores, swapchain and debug messenger

- vka\_pipeline\_t: Pipeline layout, pipeline, shader modules (note - config references descriptor set layouts, but doesn't manage them)
