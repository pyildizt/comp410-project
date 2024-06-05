#include "model_projections.hpp"
#include "vec.h"
#include <cfloat>
#include <cmath>
#include "limits.h"


// written by ChatGPT
vec2 sphereParameterization(vec3 point) {
  float u = 0.5f + (std::atan2(point.z, point.x) / (2.0f * M_PI));
  float v = 0.5f - (std::asin(point.y) / M_PI);

  return vec2(u, v);
}

// written by Copilot
void sphericalProjection(model *model, GLfloat *selected, vec4 *normals) {

  // project
  for (int i = 0; i < model->triangles.size(); i++) {
    if (selected[i * 3] == 1.0) {
      auto u0 = normalize(normals[i * 3]);
      auto u1 = normalize(normals[(i * 3) + 1]);
      auto u2 = normalize(normals[(i * 3) + 2]);
      model->triangles[i].uv0 = sphereParameterization(vec3(u0.x, u0.y, u0.z));
      model->triangles[i].uv1 = sphereParameterization(vec3(u1.x, u1.y, u1.z));
      model->triangles[i].uv2 = sphereParameterization(vec3(u2.x, u2.y, u2.z));
    }
  }
}

void cylindricalProjection(model *model, GLfloat *selected) {
  // map each point on object to a point on an imaginary cylinder
  // first find center of imaginary cylinder
  vec2 center = vec2(0.0, 0.0);
  for (int i = 0; i < model->triangles.size() * 3; i++) {
    if (selected[i] == 1.0) {
      vec4 original_point = *model->triangles[i / 3].vget(i % 3);
      center += vec2(original_point.x, original_point.z);
    }
  }
  center /= model->triangles.size() * 3;

  for (int i = 0; i < model->triangles.size() * 3; i++) {
    if (selected[i] == 1.0) {
      // map the point to a cylinder
      vec4 original_point;
      if (i % 3 == 0) {
        original_point = model->triangles[i / 3].p0;
      } else if (i % 3 == 1) {
        original_point = model->triangles[i / 3].p1;
      } else {
        original_point = model->triangles[i / 3].p2;
      }
      vec2 topdown = vec2(original_point.x, original_point.z) - center;
      topdown = normalize(topdown);
      auto cylinder_point = vec4(topdown.x, original_point.y, topdown.y, 1.0);
      auto u = (std::atan2(cylinder_point.z, cylinder_point.x) + M_PI) /
               (2.0 * M_PI);
      auto v = (cylinder_point.y + 1.0) / 2.0;

      if (i % 3 == 0) {
        model->triangles[i / 3].uv0 = vec2(u, v);
      } else if (i % 3 == 1) {
        model->triangles[i / 3].uv1 = vec2(u, v);
      } else {
        model->triangles[i / 3].uv2 = vec2(u, v);
      }
    }
  }
}

void projectFromView(model *model, GLfloat *selected, mat4 transform) {
  for (int i = 0; i < model->triangles.size() * 3; i++) {
    if (selected[i] == 1.0) {
      vec4 original_point;
      if (i % 3 == 0) {
        original_point = model->triangles[i / 3].p0;
      } else if (i % 3 == 1) {
        original_point = model->triangles[i / 3].p1;
      } else {
        original_point = model->triangles[i / 3].p2;
      }
      auto transformed_point = transform * original_point;
      auto u = (transformed_point.x + 1.0) / 2.0;
      auto v = (transformed_point.y + 1.0) / 2.0;
      if (i % 3 == 0) {
        model->triangles[i / 3].uv0 = vec2(u, v);
      } else if (i % 3 == 1) {
        model->triangles[i / 3].uv1 = vec2(u, v);
      } else {
        model->triangles[i / 3].uv2 = vec2(u, v);
      }
    }
  }
}

// unwrap the cube to a T shape
vec2 cubeToUV(vec3 point, int faceNumber) {
  vec2 uv;
  float u, v;

  // I gave one of the cases myself, for the rest of them Copilot helped
  switch (faceNumber) {
  case 0: // Front face
    u = (point.x + 1.0f) / 2.0f;
    v = (point.y + 1.0f) / 2.0f;
    uv = vec2(1.0f / 4.0f + u / 4.0f, 1.0f / 2.0f - v / 4.0f);
    break;
  case 1: // Back face
    u = (point.x + 1.0f) / 2.0f;
    v = (point.y + 1.0f) / 2.0f;
    uv = vec2(3.0f / 4.0f + u / 4.0f, 1.0f / 2.0f - v / 4.0f);
    break;
  case 2: // Left face
    u = (point.z + 1.0f) / 2.0f;
    v = (point.y + 1.0f) / 2.0f;
    uv = vec2(0.0f / 4.0f + u / 4.0f, 1.0f / 2.0f - v / 4.0f);
    break;
  case 3: // Right face
    u = (point.z + 1.0f) / 2.0f;
    v = (point.y + 1.0f) / 2.0f;
    uv = vec2(2.0f / 4.0f + u / 4.0f, 1.0f / 2.0f - v / 4.0f);
    break;
  case 4: // Top face
    u = (point.x + 1.0f) / 2.0f;
    v = (point.z + 1.0f) / 2.0f;
    uv = vec2(1.0f / 4.0f + u / 4.0f, 3.0f / 4.0f - v / 4.0f);
    break;
  case 5: // Bottom face
    u = (point.x + 1.0f) / 2.0f;
    v = (point.z + 1.0f) / 2.0f;
    uv = vec2(1.0f / 4.0f + u / 4.0f, 1.0f / 4.0f - v / 4.0f);
    break;
  default:
    // Handle invalid faceNumber
    uv = vec2(0.0f, 0.0f);
    break;
  }

  return uv;
}

// this is a long function, I took help from both Copilot and ChatGPT
void cubeProjection(model *model, GLfloat *selected) {
  // map each point on object to a point on an imaginary cube
  // first find center of imaginary cube
  vec3 center = vec3(0.0, 0.0, 0.0);
  for (int i = 0; i < model->triangles.size() * 3; i++) {
      vec4 original_point = *model->triangles[i / 3].vget(i % 3);
      center += vec3(original_point.x, original_point.y, original_point.z);
  }
  center /= model->triangles.size() * 3;
  // now find max distance from center
  float max_distance = 0.0;
  for (int i = 0; i < model->triangles.size() * 3; i++) {
      vec4 original_point = *model->triangles[i / 3].vget(i % 3);
      max_distance = std::max(
          max_distance,
          length(vec3(original_point.x, original_point.y, original_point.z) -
                 center));
  }

  // for each point, cast a ray from the center to the point, and find the
  // intersection with the cube

  for (int i = 0; i < model->triangles.size(); i++) {
    if (selected[i*3] == 1.0) {
      triangle tri = model->triangles[i];
      vec4 face_center = (tri.p0 + tri.p1 + tri.p2) / 3.0;
      vec3 direction = normalize(
          vec3(face_center.x, face_center.y, face_center.z) - center);
      
      // the line equation is center + t * direction
      // check where it collides with x = 1 or y = 1 or z = 1 or -1
    std::cout<< "DIRECTION: " << direction << std::endl;
    float t = FLT_MAX;
    if(direction.x != 0){
        auto ct = -1.0 / direction.x;
        if(ct > 0) t = std::fmin(t, ct);
        ct = 1.0 / direction.x;
        if(ct > 0) t = std::fmin(t, ct);
    }
    if(direction.y != 0){
        auto ct = -1.0 / direction.y;
        if(ct > 0) t = std::fmin(t, ct);
        ct = 1.0 / direction.y;
        if(ct > 0) t = std::fmin(t, ct);
    }
    if(direction.z != 0){
        auto ct = -1.0 / direction.z;
        if(ct > 0) t = std::fmin(t, ct);
        ct = 1.0 / direction.z;
        if(ct > 0) t = std::fmin(t, ct);
    }
    
      vec3 intersection = t * direction;
      std::cout << intersection << std::endl;
      float eps = 1e-4;
      int face = -1;
      if (std::fabs(intersection.x - 1.0) < eps) {
        // uv = (vec2(intersection.y + 1.0, intersection.z + 1.0)) / 2.0;
        // uv *= .25;
        // uv.x += .375;
        face = 0;
      } else if (std::fabs(intersection.x + 1.0) < eps) {
        // uv = (vec2(intersection.y + 1.0, intersection.z + 1.0)) / 2.0;
        // uv *= .25;
        // uv.x += .375;
        // uv.y += .25;
        face = 1;
      } else if (std::fabs(intersection.y - 1.0) < eps) {
        // uv = (vec2(intersection.z + 1.0, intersection.x + 1.0)) / 2.0;
        // uv *= .25;
        // uv.x += .375;
        // uv.y += .5;
        face = 2;
      } else if (std::fabs(intersection.y + 1.0) < eps) {
        // uv = (vec2(intersection.z + 1.0, intersection.x + 1.0)) / 2.0;
        // uv *= .25;
        // uv.x += .375 - .25;
        // uv.y += .5;
        face = 3;
      } else if (std::fabs(intersection.z - 1.0) < eps) {
        // uv = (vec2(intersection.x + 1.0, intersection.y + 1.0)) / 2.0;
        // uv *= .25;
        // uv.x += .375 + .25;
        // uv.y += .5;
        face = 4;
      } else if (std::fabs(intersection.z + 1.0) < eps) {
        // uv = (vec2(intersection.x + 1.0, intersection.y + 1.0)) / 2.0;
        // uv *= .25;
        // uv.x += .375;
        // uv.y += .75;
        face = 5;
      }
      for(int j = 0; j < 3; j++){
        vec2 uv;
        auto _point = *model->triangles[i].vget(j);
        auto _direction = normalize(vec3(_point.x, _point.y, _point.z) - center);
        // map point to the face obtained
        if(face == 0){
            float _t =  _direction.x == 0 ? 1.0 : (1.0) / _direction.x;
            auto _intersection = _t * _direction;
            uv = (vec2(_intersection.y + 1.0, _intersection.z + 1.0)) / 2.0;
            uv *= .25;
            uv.x += .375;
        }
        else if(face == 1){
            float _t =  _direction.x == 0 ? 1.0 : (-1.0) / _direction.x;
            auto _intersection = _t * _direction;
            uv = (vec2(_intersection.y + 1.0, _intersection.z + 1.0)) / 2.0;
            uv *= .25;
            uv.x += .375;
            uv.y += .25;
        }
        else if(face == 2){
            float _t =  _direction.y == 0 ? 1.0 : (1.0) / _direction.y;
            auto _intersection = _t * _direction;
            uv = (vec2(_intersection.z + 1.0, _intersection.x + 1.0)) / 2.0;
            uv *= .25;
            uv.x += .375;
            uv.y += .5;
        }
        else if(face == 3){
            float _t =  _direction.y == 0 ? 1.0 : (-1.0) / _direction.y;
            auto _intersection = _t * _direction;
            uv = (vec2(_intersection.z + 1.0, _intersection.x + 1.0)) / 2.0;
            uv *= .25;
            uv.x += .375 - .25;
            uv.y += .5;
        }
        else if(face == 4){
            float _t =  _direction.z == 0 ? 1.0 : (1.0) / _direction.z;
            auto _intersection = _t * _direction;
            uv = (vec2(_intersection.x + 1.0, _intersection.y + 1.0)) / 2.0;
            uv *= .25;
            uv.x += .375 + .25;
            uv.y += .5;
        }
        else if(face == 5){
            float _t =  _direction.z == 0 ? 1.0 : (-1.0) / _direction.z;
            auto _intersection = _t * _direction;
            uv = (vec2(_intersection.x + 1.0, _intersection.y + 1.0)) / 2.0;
            uv *= .25;
            uv.x += .375;
            uv.y += .75;
        }
      std::cout << "UV: " << uv << std::endl;
      *model->triangles[i].uget(j) = uv;
      }
    //   auto uv = cubeToUV(intersection, faceNumber);
    }
  }
}