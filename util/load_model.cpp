#include "load_model.hpp"
/// BURLARI SİNANDAN ÇALDIM
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

/**
 * The function `load` reads vertex and triangle data from a file, processes the
 * data, and returns an array of triangle points.
 *
 * @param filename The `filename` parameter in the `load` function is a pointer
 * to a constant character array that represents the name of the file from which
 * data is to be loaded.
 *
 * @return a pointer to an array of vec4 structures containing the triangle
 * points read from the file.
 */
vec4 *load(const char *filename, int *tcount) {
  std::ifstream inputFile(filename);
  if (!inputFile) {
    std::cerr << "Error opening file." << std::endl;
    return NULL;
  }

  std::string dummy;
  std::getline(inputFile, dummy);

  // Get the number of vertices and triangles
  int vertexCount, edgeCount, faceCount;
  inputFile >> vertexCount >> faceCount >> edgeCount;
  if (faceCount == 0) {
    faceCount = edgeCount / 3;
  }

  // Allocate memory for points
  vec4 *points = (vec4 *)malloc(vertexCount * sizeof(vec4));
  auto triangle_points = new std::vector<vec4>();
  float max_scale = 0;
  // Read the vertices
  for (int i = 0; i < vertexCount; i++) {
    float x, y, z;
    inputFile >> x >> y >> z;
    points[i] = vec4(x, y, z, 1.0);
    max_scale = std::max(
        max_scale, std::max(std::abs(x), std::max(std::abs(y), std::abs(z))));
  }

  // Normalize the points
  for (int i = 0; i < vertexCount; i++) {
    points[i] = points[i] / max_scale;
    points[i].w = 1.0;
  }

  // Read the triangle indices and assign the points
  for (int i = 0; i < faceCount; i++) {
    int num_verts;
    inputFile >> num_verts;
    int verts[num_verts];
    for (int j = 0; j < num_verts; j++) {
      inputFile >> verts[j];
      std::cout << verts[j] << " ";
    }
    std::cout << " AAA " << std::endl;
    // Skip rest of line in inputFile
    inputFile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    for (int j = 1; j < num_verts - 1; j++) {
      triangle_points->push_back(points[verts[0]]);
      std::cout << vertexCount << " " << verts[j] << " " << verts[j + 1]
                << std::endl;
      std::cout << vertexCount << " " << points[verts[j]] << " "
                << points[verts[j + 1]] << " // " << std::endl;

      triangle_points->push_back(points[verts[j]]);
      triangle_points->push_back(points[verts[j + 1]]);
    }
  }
  if (tcount != nullptr) {
    *tcount = triangle_points->size() / 3;
  }

  vec4 *allocated_triangles =
      (vec4 *)malloc(triangle_points->size() * sizeof(vec4));
  for (size_t i = 0; i < triangle_points->size(); i++) {
    allocated_triangles[i] = triangle_points->at(i);
  }
  delete triangle_points;
  free(points);
  return allocated_triangles;
}

std::vector<triangle> load_triangles(std::string path) {
  int tcount;
  vec4 *triangle_points = load(path.c_str(), &tcount);
  std::vector<triangle> ret = std::vector<triangle>();
  for (int i = 0; i < tcount; i++) {
    triangle t = triangle();
    t.p0 = triangle_points[i * 3];
    t.p1 = triangle_points[(i * 3) + 1];
    t.p2 = triangle_points[(i * 3) + 2];
    t.uv0 = vec2(0, 0);
    t.uv1 = vec2(1, 0);
    t.uv2 = vec2(0, 1);
    ret.push_back(t);
  }
  free(triangle_points);
  return ret;
}

model load_model_with_default_material(std::string path) {
  auto triangles = load_triangles(path);
  return load_model_with_default_material(triangles);
}

model load_model_with_default_material(std::vector<triangle> triangles) {
  model ret = model();
  ret.triangles = triangles;
  ret.material = material_properties();
  return ret;
}

void renormalizeUVs(model *_model) {
  for (int i = 0; i < _model->triangles.size(); i++) {
    triangle *t = &_model->triangles.at(i);
    auto uv0 = t->uv0;
    auto uv1 = t->uv1;
    auto uv2 = t->uv2;
    // check if uv normal is backward
    vec2 d1 = vec2(uv1 - uv0);
    vec2 d2 = vec2(uv2 - uv0);
    if (d1.x * d2.y - d1.y * d2.x < 0) {
      t->uv1 = uv2;
      t->uv2 = uv1;
    }
  }
}