#pragma once
#include "Renderer.h"

class application
{
private:
	application(int width, int height, graphics_api api);
	~application();
	static application* app_;
	SDL_Window* window_;
	renderer* renderer_;
	scene* scene_;
public:
	static int width_;
	static int height_;
	static graphics_api use_api_;
	static void pre_init(int width = 800, int height = 600, graphics_api api = graphics_api::vulkan);
	static application* get_instanced();
	void run() const;
	static void destroy()
	{
		delete application::app_;
	}
};
