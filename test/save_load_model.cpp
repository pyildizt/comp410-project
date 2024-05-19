#include "data_types.hpp"
#include "load_model.hpp"
#include <cassert>
#include <fstream>
#include <sstream>


bool point_close_enough(vec4 p1, vec4 p2) {
  return length(p1 - p2) < 0.001;
}
bool point_close_enough(vec2 p1, vec2 p2) {
  return length(p1 - p2) < 0.001;
}
bool point_close_enough(float p1, float p2) {
  return abs(p1 - p2) < 0.001;
}

/**
 * @brief Initial test to load a bunny model, save it as json in a file.
 *
 */
void test1() {
  auto model = load_model_with_default_material("./cube.off");
  auto json = model_to_json(model);
  std::ofstream file("cube.json");
  file << json;
  file.close();
}

/**
 * @brief Load a saved json, compare against the original model
 *
 */
void test2() {

  auto model = load_model_with_default_material("./mushroom.off");
  auto json = model_to_json(model);
  std::ofstream file("mushroom.json");
  file << json;
  file.close();

  std::ifstream file2("mushroom.json");
  std::stringstream buffer;
  buffer << file2.rdbuf();
  std::string json2 = buffer.str();
  file2.close();

  auto model2 = json_to_model(json2);
  assert(model.triangles.size() == model2.triangles.size());
for (int i = 0; i < model.triangles.size(); i++) {
    assert(point_close_enough(model.triangles[i].p0, model2.triangles[i].p0));
    assert(point_close_enough(model.triangles[i].p1, model2.triangles[i].p1));
    assert(point_close_enough(model.triangles[i].p2, model2.triangles[i].p2));
    assert(point_close_enough(model.triangles[i].uv0, model2.triangles[i].uv0));
    assert(point_close_enough(model.triangles[i].uv1, model2.triangles[i].uv1));
    assert(point_close_enough(model.triangles[i].uv2, model2.triangles[i].uv2));
}
assert(point_close_enough(model.material.ambient, model2.material.ambient));
assert(point_close_enough(model.material.diffuse, model2.material.diffuse));
assert(point_close_enough(model.material.specular, model2.material.specular));
assert(point_close_enough(model.material.shininess, model2.material.shininess));
assert(model.material.texture_path == model2.material.texture_path);
}

int main(int argc, char *argv[]) {
  test1();
  std::cout << "Test 1 passed\n";
  test2();
  std::cout << "Test 2 passed\n";
}