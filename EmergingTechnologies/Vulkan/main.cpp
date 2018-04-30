#include "vulkan_application.h"
#include <iostream>

/**
 * \brief The entry point of the Vulkan Example
 * \return 1 if the application failed to run, 0 otherwise
 */
int main(int argc, char * argv[])
{
	vulkan_application app;

	try
	{
		app.run();
	}
	catch (const std::runtime_error& e)
	{
		std::cout << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
