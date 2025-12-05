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

VKA\_MAX\_FRAMES\_IN\_FLIGHT set to 2 by default - this can be overridden in the Makefile.

## Compilation:

Depends on [Config](https://github.com/Nell-Mills/Config). File expected: NM-Config/Config.h. Add this to Include directory.  
Add Include directory to search path: -I Include  
Add Libs to library search path: -L Libs -Wl,-rpath '$$ORIGIN/Libs'  
Link SDL3: -lSDL3

## Usage:

Populate a vka\_vulkan\_t struct with configuration - see the vka\_vulkan\_config\_t struct.  
Use vka\_setup\_vulkan() and vka\_shutdown_\vulkan().  
