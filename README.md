# Vulkan Abstraction

For using Vulkan 1.3 and SDL3 in my projects.  
For debug, define VKA_DEBUG when compiling.

## Functionality implemented:

- Vulkan base setup:
    - Window creation (SDL3)
    - Physical device selection and device creation
    - Graphics and present queue
    - Command pool, command buffers and fences
    - Semaphores
    - Swapchain
    - Debug messenger (with VKA_DEBUG)

- Pipeline setup:
    - Pipeline layout
    - Pipeline
    - Shader modules

- Command buffers:
    - Begin, end, submit

- Rendering:
    - Dynamic rendering - begin, end
    - Dynamic state - set viewport and scissor
    - Present swapchain image

## Compilation:

Depends on:

- [Config](https://github.com/Nell-Mills/Config) - expects <NM-Config/Config.h\>
- Volk (included)
- SDL3

Add "Include" to search path: "-I Include"  
Link SDL3: -lSDL3

## Usage:

Follow this pattern:

- For the main Vulkan base struct (vka\_vulkan\_t), define its configuration members (optional), then use vka\_setup\_vulkan() and vka\_shutdown\_vulkan(). Function arguments:
    - vka\_vulkan\_t \*

- For other structs (vka\_X\_t), there will be vka\_create\_X() and vka\_destroy\_X() functions, and usually some configuration members for you to define (again, optional). Function arguments:
    - vka\_vulkan\_t \*
    - vka\_X\_t \*

Where applicable, vka\_create\_X() functions will also destroy old resources - if the creation fails, the old resource will still be valid.

Structs available:

- vka\_vulkan\_t: Device, queues, per-frame command buffers, semaphores, swapchain and debug messenger

- vka\_command\_buffer\_t: Command buffer and fence pair

- vka\_pipeline\_t: Pipeline layout, pipeline, shaders (note - references descriptor set layouts, but doesn't manage them)

Misc structs (don't follow the pattern):

- vka\_shader\_t: Just a wrapper for a path and a shader module - managed by pipeline

- vka\_render\_info\_t: Container for information, such as render area
