#ifndef _MODEL_PROJECTIONS_HPP

#define _MODEL_PROJECTIONS_HPP

#include "data_types.hpp"

/**
 * @brief Projects the model's UV coordinates to an imaginary sphere
 * Detects the sphere's radius automatically
 * 
 * @param model model to be projected, UVs are modified inplace
 * @param selected selected vertices to be projected
 * @param normals normals of the model one for each point
 */
void sphericalProjection(model* model, GLfloat* selected, vec4* normals);

/**
 * @brief Projects the model's uvs on a cylinder
 * 
 * @param model model to be projected, UVs are modified inplace
 * @param selected 
 * @param normals 
 */
void cylindricalProjection(model* model, GLfloat* selected);

/**
 * @brief Projects the selected vertices from the view to the model UV
 * 
 * @param model 
 * @param selected 
 * @param transform 
 */
void projectFromView(model* model, GLfloat* selected, mat4 transform);


void cubeProjection(model* model, GLfloat* selected);
#endif