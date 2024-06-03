#include "data_types.hpp"
#include "vec.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <utility>
#include <vector>

mat4 to_mat4(serializable_mat4 s){
    return mat4(to_vec4(s.c0), to_vec4(s.c1), to_vec4(s.c2), to_vec4(s.c3));
}

serializable_mat4 to_serializable_mat4(mat4 m){
    struct serializable_mat4 ret = serializable_mat4();
    ret.c0 = to_serializable_vec4(m[0]);
    ret.c1 = to_serializable_vec4(m[1]);
    ret.c2 = to_serializable_vec4(m[2]);
    ret.c3 = to_serializable_vec4(m[3]);
    return ret;
}


vec4 to_vec4(serializable_vec4 s){
    return vec4(s.x, s.y, s.z, s.w);
}

serializable_vec4 to_serializable_vec4(vec4 v){
    struct serializable_vec4 ret = serializable_vec4();
    ret.x = v.x;
    ret.y = v.y;
    ret.z = v.z;
    ret.w = v.w;
    return ret;
}


vec2 to_vec2(serializable_vec2 s){
    return vec2(s.x, s.y);
}

serializable_vec2 to_serializable_vec2(vec2 v){
    struct serializable_vec2 ret = serializable_vec2();
    ret.x = v.x;
    ret.y = v.y;
    return ret;
}


triangle to_triangle(serializable_triangle s){
    struct triangle ret = triangle();
    ret.p0 = to_vec4(s.p0);
    ret.p1 = to_vec4(s.p1);
    ret.p2 = to_vec4(s.p2);

    ret.uv0 = to_vec2(s.uv0);
    ret.uv1 = to_vec2(s.uv1);
    ret.uv2 = to_vec2(s.uv2);

    return ret;
}

serializable_triangle to_serializable_triangle(triangle t){
    struct serializable_triangle ret = serializable_triangle();
    ret.p0 = to_serializable_vec4(t.p0);
    ret.p1 = to_serializable_vec4(t.p1);
    ret.p2 = to_serializable_vec4(t.p2);

    ret.uv0 = to_serializable_vec2(t.uv0);
    ret.uv1 = to_serializable_vec2(t.uv1);
    ret.uv2 = to_serializable_vec2(t.uv2);
    return ret;
}

material_properties to_material_properties(serializable_material_properties s){
    struct material_properties ret = material_properties();
    ret.ambient = to_vec4(s.ambient);
    ret.diffuse = to_vec4(s.diffuse);
    ret.specular = to_vec4(s.specular);
    ret.shininess = s.shininess;
    ret.texture_path = s.texture_path;
    return ret;
}

serializable_material_properties to_serializable_material_properties(material_properties m){
    struct serializable_material_properties ret = serializable_material_properties();
    ret.ambient = to_serializable_vec4(m.ambient);
    ret.diffuse = to_serializable_vec4(m.diffuse);
    ret.specular = to_serializable_vec4(m.specular);
    ret.shininess = m.shininess;
    ret.texture_path = m.texture_path;
    return ret;
}

scene to_scene(serializable_scene s){
    struct scene ret = scene();
    ret.models = std::vector<model>();
    for(auto m : s.models){
        ret.models.push_back(to_model(m));
    }
    ret.transforms = std::vector<mat4>();
    for(auto m : s.transforms){
        ret.transforms.push_back(to_mat4(m));
    }
    ret.view = to_mat4(s.view);
    ret.scene_light = to_light(s.scene_light);
    return ret;
}
serializable_scene to_serializable_scene(scene s){
    struct serializable_scene ret = serializable_scene();
    ret.models = std::vector<serializable_model>();
    for(auto m : s.models){
        ret.models.push_back(to_serializable_model(m));
    }
    ret.transforms = std::vector<serializable_mat4>();
    for(auto m : s.transforms){
        ret.transforms.push_back(to_serializable_mat4(m));
    }
    ret.view = to_serializable_mat4(s.view);
    ret.scene_light = to_serializable_light(s.scene_light);
    return ret;
}

model to_model(serializable_model s){
    struct model ret = model();
    ret.material = to_material_properties(s.material);
    ret.triangles = std::vector<triangle>();
    for(auto t : s.triangles){
        ret.triangles.push_back(to_triangle(t));
    }
    return ret;
}
serializable_model to_serializable_model(model m){
    struct serializable_model ret = serializable_model();
    ret.material = to_serializable_material_properties(m.material);
    ret.triangles = std::vector<serializable_triangle>();
    for(auto t : m.triangles){
        ret.triangles.push_back(to_serializable_triangle(t));
    }
    return ret;
}

light to_light(serializable_light s){
    struct light ret = light();
    ret.position = to_vec4(s.position);
    ret.ambient = to_vec4(s.ambient);
    ret.diffuse = to_vec4(s.diffuse);
    ret.specular = to_vec4(s.specular);
    ret.constant = s.constant;
    ret.linear = s.linear;
    ret.quadratic = s.quadratic;
    return ret;
}
serializable_light to_serializable_light(light l){
    struct serializable_light ret = serializable_light();
    ret.position = to_serializable_vec4(l.position);
    ret.ambient = to_serializable_vec4(l.ambient);
    ret.diffuse = to_serializable_vec4(l.diffuse);
    ret.specular = to_serializable_vec4(l.specular);
    ret.constant = l.constant;
    ret.linear = l.linear;
    ret.quadratic = l.quadratic;
    return ret;
}



std::string model_to_json(model m){
    return to_serializable_model(m).zax_to_json();
}
model json_to_model(std::string json){
    serializable_model s;
    s.zax_from_json(json.c_str());
    return to_model(s);
}

void to_point_array(vec4** points, int* len, std::vector<triangle>* triangles){
    *points = (vec4*)calloc(triangles->size() * 3, sizeof(vec4));
    *len = triangles->size() * 3;
    for(int i = 0; i < triangles->size(); i++){
        (*points)[i * 3] = (*triangles)[i].p0;
        (*points)[i * 3 + 1] = (*triangles)[i].p1;
        (*points)[i * 3 + 2] = (*triangles)[i].p2;
    }
}

void to_point_array_uv(vec2 **points, std::vector<triangle> *triangles){
    *points = (vec2 *)calloc(triangles->size() * 3, sizeof(vec2));
    for (int i = 0; i < triangles->size(); i++) {
        (*points)[i * 3] = (*triangles)[i].uv0;
        (*points)[i * 3 + 1] = (*triangles)[i].uv1;
        (*points)[i * 3 + 2] = (*triangles)[i].uv2;
    }
}

void generate_normals(vec4 *normals, const model *m, bool smooth){
    if(smooth){
        generate_smooth_normals(normals, m);
    }else{
        generate_flat_normals(normals, m);
    }
}
bool _compare_vec4_ci(vec4 a, vec4 b){
    return length(a - b) < 0.001;
}

#define COMPARE_MAX(max_val, t)                                                \
  {                                                                            \
    if (max_val < t.p0.x) {                                                    \
      max_val = t.p0.x;                                                        \
    }                                                                          \
    if (max_val < t.p1.x) {                                                    \
      max_val = t.p1.x;                                                        \
    }                                                                          \
    if (max_val < t.p2.x) {                                                    \
      max_val = t.p2.x;                                                        \
    }                                                                          \
    if (max_val < t.p0.y) {                                                    \
      max_val = t.p0.y;                                                        \
    }                                                                          \
    if (max_val < t.p1.y) {                                                    \
      max_val = t.p1.y;                                                        \
    }                                                                          \
    if (max_val < t.p2.y) {                                                    \
      max_val = t.p2.y;                                                        \
    }                                                                          \
    if (max_val < t.p0.z) {                                                    \
      max_val = t.p0.z;                                                        \
    }                                                                          \
    if (max_val < t.p1.z) {                                                    \
      max_val = t.p1.z;                                                        \
    }                                                                          \
    if (max_val < t.p2.z) {                                                    \
      max_val = t.p2.z;                                                        \
    }                                                                          \
  }

#define COMPARE_MIN(min_val, t)                                                \
  {                                                                            \
    if (min_val > t.p0.x) {                                                    \
      min_val = t.p0.x;                                                        \
    }                                                                          \
    if (min_val > t.p1.x) {                                                    \
      min_val = t.p1.x;                                                        \
    }                                                                          \
    if (min_val > t.p2.x) {                                                    \
      min_val = t.p2.x;                                                        \
    }                                                                          \
    if (min_val > t.p0.y) {                                                    \
      min_val = t.p0.y;                                                        \
    }                                                                          \
    if (min_val > t.p1.y) {                                                    \
      min_val = t.p1.y;                                                        \
    }                                                                          \
    if (min_val > t.p2.y) {                                                    \
      min_val = t.p2.y;                                                        \
    }                                                                          \
    if (min_val > t.p0.z) {                                                    \
      min_val = t.p0.z;                                                        \
    }                                                                          \
    if (min_val > t.p1.z) {                                                    \
      min_val = t.p1.z;                                                        \
    }                                                                          \
    if (min_val > t.p2.z) {                                                    \
      min_val = t.p2.z;                                                        \
    }                                                                          \
  }

#define HASH_VEC4(v4, max_val, min_val)                                        \
  (((v4.x - min_val) + ((v4.y - min_val) * max_val) +                          \
    ((v4.z - min_val) * std::pow(max_val, 2))))

void generate_smooth_normals(vec4 *normals, const model *m) {
    float _max_val = 0.0;
    float _min_val = 999.9;
    for (auto t : m->triangles) {
        COMPARE_MAX(_max_val, t);
        COMPARE_MIN(_min_val, t);
    }
    float max_val = std::ceil(_max_val);
    float min_val = std::floor(_min_val);
    std::cout << "Max value is: " << _max_val << " " << max_val;
    std::cout << "Min value is: " << _min_val << " " << min_val;

    vec4 *normal_temp = (vec4 *)calloc(m->triangles.size() * 3, sizeof(vec4));
    int idx = 0;
    auto normal_map = std::map<float, int>();
    auto normal_size_map = std::map<float, float>();

    for (auto t : m->triangles) {
        vec4 normal = cross(t.p1 - t.p0, t.p2 - t.p0);
        vec4 points[] = {t.p0, t.p1, t.p2};
        for (auto point : points) {
          if (normal_map.find(HASH_VEC4(point, max_val, min_val)) ==
              normal_map.end()) {
            // element not in map
            memcpy(normal_temp + idx, &normal, sizeof(vec4));
            normal_map[HASH_VEC4(point, max_val, min_val)] = idx;
            idx++;
            normal_size_map[HASH_VEC4(point, max_val, min_val)] =
                length(vec3(normal.x, normal.y, normal.z));
          } else {
            // element in the map
            int id = normal_map[HASH_VEC4(point, max_val, min_val)];
            vec4 addition = normal + normal_temp[id];
            memcpy(normal_temp + id, &addition, sizeof(vec4));
            normal_size_map[HASH_VEC4(point, max_val, min_val)] =
                normal_size_map[HASH_VEC4(point, max_val, min_val)] +
                length(vec3(normal.x, normal.y, normal.z));
          }
        }
    }
    std::cout << "First k and v" << normal_size_map.begin()->first << " "
              << normal_size_map.begin()->second << std::endl;

    for (size_t i = 0; i < m->triangles.size(); i++) {
        vec4 points[] = {m->triangles.at(i).p0, m->triangles.at(i).p1,
                         m->triangles.at(i).p2};
        for (int j = 0; j < 3; j++) {
          vec4 normal =
              normal_temp[normal_map[HASH_VEC4(points[j], max_val, min_val)]];
          normals[(i * 3) + j] =
              normal / normal_size_map[HASH_VEC4(points[j], max_val, min_val)];
          normals[(i * 3) + j].w = 0;
        }
    }
    free(normal_temp);
}

void generate_flat_normals(vec4 *normals, const model *m){
    for(int i = 0; i < m->triangles.size(); i++){
        vec4 normal = cross(m->triangles[i].p1 - m->triangles[i].p0, m->triangles[i].p2 - m->triangles[i].p0);
        normals[i * 3] = normal;
        normals[i * 3 + 1] = normal;
        normals[i * 3 + 2] = normal;
    }
}


void invert_normals(model* m){
    for(size_t i = 0; i < m->triangles.size(); i++){
      vec4 temp = m->triangles[i].p1;
      m->triangles[i].p1 = m->triangles[i].p2;
      m->triangles[i].p2 = temp;
      // also invert UVs
      // vec2 temp_uv = m->triangles[i].uv1;
      // m->triangles[i].uv1 = m->triangles[i].uv2;
      // m->triangles[i].uv2 = temp_uv;
    }
}

