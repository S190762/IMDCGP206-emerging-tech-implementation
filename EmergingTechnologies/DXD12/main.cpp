#include "d3dx12_application.h"

/**
* \brief The entry point of the D3DX12 Example
* \return 1 if the application failed to run, 0 otherwise
*/
int main(int argc, char * argv[])
{
	d3dx12_application app;

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
