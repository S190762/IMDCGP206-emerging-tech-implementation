#include "Application.h"
#include "VulkanRenderer.h"
#include <SDL.h>
#include <SDL_vulkan.h>
#include <iostream>

application * application::app_ = nullptr;
int application::width_ = 0;
int application::height_ = 0;
graphics_api application::use_api_ = graphics_api::vulkan;

application::application(const int width, const int height, graphics_api api)
{
	SDL_Init(SDL_INIT_VIDEO);
	window_ = SDL_CreateWindow("Vulkan", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_VULKAN);
	switch (use_api_)
	{
	case graphics_api::vulkan:
		renderer_ = new vulkan_renderer(window_, scene_);
		break;
	case graphics_api::direct_x12:
		std::cout << "api not supported";
		break;
	default:
		std::cout << "api not supported";
		break;
	}
}

application::~application()
{
	delete scene_;
	delete renderer_;
	SDL_DestroyWindow(window_);
	SDL_Quit();
}

void application::pre_init(const int width, const int height, const graphics_api api)
{
	application::width_ = width;
	application::height_ = height;
	application::use_api_ = api;
}

application* application::get_instanced()
{
	if(application::app_ == nullptr)
	{
		application::app_ = new application(
			application::width_,
			application::height_,
			application::use_api_
		);
	}
	return application::app_;
}

void application::run() const
{
	auto running = true;
	while(running)
	{
		SDL_Event event;
		while(SDL_PollEvent(&event))
		{
			if(event.type == SDL_QUIT)
			{
				running = false;
			}
		}
		renderer_->update();
		renderer_->render();
	}
}
