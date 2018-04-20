#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "SceneUtil.h"

class scene
{
public:
	std::vector<glm::ivec3> indices;
	std::vector<glm::vec4> vertexPositions;
	std::vector<glm::vec4> vertexNormals;
	std::vector<glm::vec2> vertexUVs;
	std::vector<mesh_data*> meshesData;
	scene()
	{
		
	}
	~scene()
	{
		for(mesh_data* geom : meshesData)
		{
			delete geom;
			geom = nullptr;
		}
	}
};
