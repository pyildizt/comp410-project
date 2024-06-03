#ifndef DATA_TYPES
#define DATA_TYPES
#include "Angel.h"
#include <climits>
#include <cstdio>
#include <cstring>
#include <map>
#include <stdlib.h>
#include <string>
#include <vector>
#include <limits>
#include "../extern/zax-parser/src/ZaxJsonParser.h"

struct serializable_vec4 {
  float x;
  float y;
  float z;
  float w;

  ZAX_JSON_SERIALIZABLE_BASIC(JSON_PROPERTY(x), JSON_PROPERTY(y),
                              JSON_PROPERTY(z), JSON_PROPERTY(w))
};

struct serializable_mat4 {
  serializable_vec4 c0;
  serializable_vec4 c1;
  serializable_vec4 c2;
  serializable_vec4 c3;

  ZAX_JSON_SERIALIZABLE_BASIC(JSON_PROPERTY(c0), JSON_PROPERTY(c1),
                              JSON_PROPERTY(c2), JSON_PROPERTY(c3))
};

mat4 to_mat4(serializable_mat4 s);
serializable_mat4 to_serializable_mat4(mat4 m);

struct serializable_vec2 {
  float x;
  float y;

  ZAX_JSON_SERIALIZABLE_BASIC(JSON_PROPERTY(x), JSON_PROPERTY(y))
};

vec4 to_vec4(serializable_vec4 s);
serializable_vec4 to_serializable_vec4(vec4 v);

vec2 to_vec2(serializable_vec2 s);
serializable_vec2 to_serializable_vec2(vec2 v);

struct serializable_triangle {
  serializable_vec4 p0;
  serializable_vec4 p1;
  serializable_vec4 p2;

  // uv coordinates
  serializable_vec2 uv0;
  serializable_vec2 uv1;
  serializable_vec2 uv2;

  ZAX_JSON_SERIALIZABLE_BASIC(JSON_PROPERTY(p0), JSON_PROPERTY(p1),
                              JSON_PROPERTY(p2), JSON_PROPERTY(uv0),
                              JSON_PROPERTY(uv1), JSON_PROPERTY(uv2))
};

struct triangle {
  vec4 p0;
  vec4 p1;
  vec4 p2;

  // uv coordinates
  vec2 uv0;
  vec2 uv1;
  vec2 uv2;

  vec4* vget(int i) {
    switch (i) {
    case 0:
      return &p0;
    case 1:
      return &p1;
    case 2:
      return &p2;
    default:
      return nullptr;
    }
  }

  vec2* uget(int i) {
    switch (i) {
    case 0:
      return &uv0;
    case 1:
      return &uv1;
    case 2:
      return &uv2;
    default:
      return nullptr;
    }
  }
};

triangle to_triangle(serializable_triangle s);
serializable_triangle to_serializable_triangle(triangle t);

struct material_properties {
  vec4 ambient;
  vec4 diffuse;
  vec4 specular;
  float shininess;
  char texture_path[1024];

  material_properties() {
    // default gray material
    ambient = vec4(0.4, 0.4, 0.4, 1.0);
    diffuse = vec4(0.8, 0.8, 0.8, 1.0);
    specular = vec4(0.3, 0.3, 0.3, 1.0);
    shininess = 10.0;
    memset(texture_path, 0, 1024);
  }
};

struct serializable_material_properties {
  serializable_vec4 ambient;
  serializable_vec4 diffuse;
  serializable_vec4 specular;
  float shininess;
  std::string texture_path;

  ZAX_JSON_SERIALIZABLE_BASIC(JSON_PROPERTY(ambient), JSON_PROPERTY(diffuse),
                              JSON_PROPERTY(specular), JSON_PROPERTY(shininess),
                              JSON_PROPERTY(texture_path))
};

material_properties to_material_properties(serializable_material_properties s);
serializable_material_properties
to_serializable_material_properties(material_properties m);

struct serializable_model {
  std::vector<serializable_triangle> triangles;
  serializable_material_properties material;
  ZAX_JSON_SERIALIZABLE(serializable_model, JSON_PROPERTY(triangles),
                        JSON_PROPERTY(material))
};

struct model {
  std::vector<triangle> triangles;
  material_properties material;

  model() {
    triangles = std::vector<triangle>();
    material = material_properties();
  }

  model(std::vector<triangle> t, material_properties m) {
    triangles = t;
    material = m;
  }

  bool is_empty() { return triangles.size() == 0; }
};

struct light{
  vec4 position;
  vec4 ambient;
  vec4 diffuse;
  vec4 specular;
  float constant;
  float linear;
  float quadratic;

  light(){
    position = vec4(0.0, 0.0, 0.0, 1.0);
    ambient = vec4(0.2, 0.2, 0.2, 1.0);
    diffuse = vec4(0.5, 0.5, 0.5, 1.0);
    specular = vec4(1.0, 1.0, 1.0, 1.0);
    constant = 1.0;
    linear = 0.09;
    quadratic = 0.032;
  }
};


struct serializable_light{
  serializable_vec4 position;
  serializable_vec4 ambient;
  serializable_vec4 diffuse;
  serializable_vec4 specular;
  float constant;
  float linear;
  float quadratic;

  ZAX_JSON_SERIALIZABLE_BASIC(JSON_PROPERTY(position), JSON_PROPERTY(ambient),
                              JSON_PROPERTY(diffuse), JSON_PROPERTY(specular),
                              JSON_PROPERTY(constant), JSON_PROPERTY(linear),
                              JSON_PROPERTY(quadratic))
};


struct scene{
  std::vector<model> models;
  std::vector<mat4> transforms;
  mat4 view;
  light scene_light;
};

struct serializable_scene{
  std::vector<serializable_model> models;
  std::vector<serializable_mat4> transforms;
  serializable_mat4 view;
  serializable_light scene_light;

  ZAX_JSON_SERIALIZABLE(serializable_scene, JSON_PROPERTY(models),
                        JSON_PROPERTY(transforms), JSON_PROPERTY(view),
                        JSON_PROPERTY(scene_light))
};

scene to_scene(serializable_scene s);
serializable_scene to_serializable_scene(scene s);

model to_model(serializable_model s);
serializable_model to_serializable_model(model m);

std::string model_to_json(model m);
model json_to_model(std::string json);

light to_light(serializable_light s);
serializable_light to_serializable_light(light l);


enum ObjectType {Cube, Sphere, Imported, PointLight, Empty};
enum {Xaxis = 0, Yaxis = 1, Zaxis = 2, NumAxes = 3};
struct object_model
{
    enum ObjectType object_type;
    bool is_selected;
    int shaded_object_index;
    vec4 unique_id_color;
    vec3 object_coordinates;
    mat4 model_matrix;
    mat4 rotation_matrix;
    GLfloat Theta[NumAxes];
    GLfloat Scaling[NumAxes];
    GLfloat Translation[NumAxes];

    friend std::ostream& operator<<(std::ostream& os, const object_model& obj) {
        vec3 coords = obj.object_coordinates + vec3(obj.Translation[Xaxis], obj.Translation[Yaxis], obj.Translation[Zaxis]);
        os << "is_selected: " << obj.is_selected << ", object_type: " << obj.object_type << 
            ", coords: (" << coords.x << ", " << coords.y << ", " << coords.z << ")";
        return os;
    }
};




/**
 * @brief Converts a vector of triangles to a point array to be used in opengl.
 * Don't forget to free the memory allocated for the points array afterwards.
 *
 * @param points OUT: array of points to be used in opengl
 * @param len OUT: length of the points array
 * @param triangles IN: vector of triangles
 */
void to_point_array(vec4 **points, int *len, std::vector<triangle> *triangles);

/**
 * @brief Converts a vector of triangles to a point array to be used in opengl with their UV coordinates.
 * Don't forget to free the memory allocated for the points array afterwards.
 *
 * @param points OUT: array of points to be used in opengl
 * @param triangles IN: vector of triangles
 */
void to_point_array_uv(vec2 **points, std::vector<triangle> *triangles);


/**
 * @brief Generates normals for the model and puts them in the normals array.
 *
 * @param normals IN: normals array, should have same size of
 * model.triangles.size()
 * @param m IN: model to generate normals for
 * @param smooth IN: if true, normals are generated per vertex, if false,
 * normals are generated per face
 */
void generate_normals(vec4 *normals, const model *m, bool smooth);
void generate_smooth_normals(vec4 *normals, const model *m);
void generate_flat_normals(vec4 *normals, const model *m);

void invert_normals(model* m);
#endif