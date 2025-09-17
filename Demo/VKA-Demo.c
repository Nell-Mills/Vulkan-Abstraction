#include "../Vulkan-Abstraction.h"

int main(int argc, char **argv)
{
	vka_vulkan_t vulkan = vka_vulkan_initialise();
	int error = vka_vulkan_setup(&vulkan);
	if (error)
	{
		vka_vulkan_shutdown(&vulkan);
		return -1;
	}
	#ifdef VKA_DEBUG
	vka_print_vulkan(&vulkan, stdout);
	#endif
	vka_vulkan_shutdown(&vulkan);

	printf("Error: %s\n", vulkan.error_message);

	return 0;
}
