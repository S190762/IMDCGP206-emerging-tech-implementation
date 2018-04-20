#include "Application.h"

int main(int argc, char** argv)
{
	application::pre_init(1024, 768, graphics_api::vulkan);
	application::get_instanced()->run();
	application::destroy();
	return 1;
}