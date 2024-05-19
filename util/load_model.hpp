#ifndef LOAD_MODEL
#define LOAD_MODEL

#include "data_types.hpp"

/**
 * @brief Loads triangles from an .off file. By default, the UV coordinates of all triangles are just a triangle with vertices (0, 0), (0, 1), (1, 0)
 * 
 * @param path path to off file
 * @return std::vector<triangle> 
 */
std::vector<triangle> load_triangles(std::string path);
model load_model_with_default_material(std::string path);
model load_model_with_default_material(std::vector<triangle> triangles);


void renormalizeUVs(model* _model);

#endif