#pragma once
#include <map>
#include <vector>

typedef enum
{
	INDEX,
	POSITION,
	NORMAL,
	TEXCOORD
} vertex_attribute;

typedef struct vertex_attribute_info_t {
	size_t byte_stride;
	size_t count;
	int comp_length;
	int comp_type_byte_size;
} vertex_attribute_info;

struct mesh_data
{
	std::map<vertex_attribute, std::vector<unsigned char>> vertex_data;
	std::map<vertex_attribute, vertex_attribute_info> attrib_info;
};