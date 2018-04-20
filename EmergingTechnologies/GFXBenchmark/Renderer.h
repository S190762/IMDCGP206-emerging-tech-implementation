#pragma once
#include <SDL.h>
#include "Scene.h"

typedef enum
{
	vulkan,
	direct_x12
} graphics_api;

class renderer
{
protected:
	SDL_Window* window_;
	scene* scene_;
	uint32_t width_;
	uint32_t height_;
public:
	renderer(SDL_Window* window, scene* scene) : window_(window), scene_(scene)
	{
		int width, height;
		SDL_GetWindowSize(window, &width, &height);
		width_ = width;
		height_ = height;
	}

	virtual ~renderer() {}
	virtual void update() = 0;
	virtual void render() = 0;
};