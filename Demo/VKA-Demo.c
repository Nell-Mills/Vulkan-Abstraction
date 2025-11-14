#include "../Vulkan-Abstraction.h"

int main(int argc, char **argv)
{
	vka_vulkan_t vulkan = vka_vulkan_initialise();
	VkResult error = vka_vulkan_setup(&vulkan);
	if (error)
	{
		printf("Error: %s", vulkan.error_message);
		vka_vulkan_shutdown(&vulkan);
		return -1;
	}
	#ifdef VKA_DEBUG
	vka_vulkan_print(stdout, &vulkan);
	#endif

	SDL_Event event;
	int running = 1;
	while(running)
	{
		while (SDL_PollEvent(&event) != 0)
		{
			if (event.type == SDL_EVENT_QUIT) { running = 0; }
		}
	}

	vka_vulkan_shutdown(&vulkan);
	if (strcmp(vulkan.error_message, "") != 0) { printf("Error: %s", vulkan.error_message); }
	return 0;
}
