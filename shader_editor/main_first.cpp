#include "data_types.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "load_model.hpp"
#include "mat.h"
#include "model_projections.hpp"
#include "png_utils.hpp"
#include "simple_windowing.hpp"
#include "vec.h"
#include <GL/gl.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>

#define INT_TO_UNIQUE_COLOR(x)                                                 \
  vec4(((GLfloat)(x & 0xFF)) / 255.0, ((GLfloat)((x >> 8) & 0xFF)) / 255.0,    \
       ((GLfloat)((x >> 16) & 0xFF)) / 255.0, 1.0)

#define UNIQUE_COLOR_TO_INT(c)                                                 \
  ((int)(c.x * 255) + ((int)(c.y * 255) << 8) + ((int)(c.z * 255) << 16))

enum axis { Rx, Ry, Rz };
#define AXIS_STR(x) ((x == Rx) ? "Rx" : (x == Ry) ? "Ry" : "Rz")
/******************************************************************/
//---------------------------------------------------------------------------

namespace SelectedWindow {
int selected = -1;
enum SelectedType {
  OBJECT,
  UV,
};
} // namespace SelectedWindow

subwindow main_window = subwindow(500.0, 500.0);

subwindow uv_window(elastic_scale(elastic_value(120, swind::ABSOLUTE),
                                  elastic_value(100, swind::PERCENT),
                                  elastic_value(40, swind::PERCENT)),
                    elastic_scale(elastic_value(100, swind::PERCENT),
                                  elastic_value(100, swind::PERCENT),
                                  elastic_value(100, swind::PERCENT)),
                    elastic_scale(elastic_value(0, swind::ABSOLUTE),
                                  elastic_value(0, swind::ABSOLUTE),
                                  elastic_value(0, swind::ABSOLUTE)),
                    elastic_scale(elastic_value(0, swind::ABSOLUTE),
                                  elastic_value(0, swind::ABSOLUTE),
                                  elastic_value(0, swind::ABSOLUTE)),
                    &main_window, nullptr);

subwindow object_window(elastic_scale(elastic_value(120, swind::ABSOLUTE),
                                      elastic_value(100, swind::PERCENT),
                                      elastic_value(40, swind::PERCENT)),
                        elastic_scale(elastic_value(100, swind::PERCENT),
                                      elastic_value(100, swind::PERCENT),
                                      elastic_value(100, swind::PERCENT)),
                        elastic_scale(elastic_value(0, swind::ABSOLUTE),
                                      elastic_value(0, swind::ABSOLUTE),
                                      elastic_value(0, swind::ABSOLUTE)),
                        elastic_scale(elastic_value(0, swind::ABSOLUTE),
                                      elastic_value(0, swind::ABSOLUTE),
                                      elastic_value(0, swind::ABSOLUTE)),
                        &main_window, &uv_window);

subwindow simple_ui_window(elastic_scale(elastic_value(120, swind::ABSOLUTE),
                                         elastic_value(100, swind::PERCENT),
                                         elastic_value(20, swind::PERCENT)),
                           elastic_scale(elastic_value(100, swind::PERCENT),
                                         elastic_value(100, swind::PERCENT),
                                         elastic_value(100, swind::PERCENT)),
                           elastic_scale(elastic_value(0, swind::ABSOLUTE),
                                         elastic_value(0, swind::ABSOLUTE),
                                         elastic_value(0, swind::ABSOLUTE)),
                           elastic_scale(elastic_value(0, swind::ABSOLUTE),
                                         elastic_value(0, swind::ABSOLUTE),
                                         elastic_value(0, swind::ABSOLUTE)),
                           &main_window, &object_window);

/**
 * Camera Stuff
 *
 */

mat4 projection = identity();
mat4 cameraTranslate = Translate(0.0, 0.0, -2.0);
mat4 cameraXRotation = identity();
mat4 cameraYRotation = identity();
GLfloat cameraXrotacc = 0.0;

GLuint _width;
GLuint _height;

// We want camera to be able to rotate around the origin, with certain distance

mat4 getCameraInverse() {
  return cameraTranslate * cameraXRotation * cameraYRotation;
}

mat4 on_get_leftsize_ortho(GLint width, GLint height, subwindow *window);

GLuint cameraProjection;
GLuint cameraTransform;
GLfloat cameraDistance = 2.0;

void rotateCamera(GLuint axis, GLdouble angle) {
  switch (axis) {
  case Rx:
    if (cameraXrotacc + angle > 75 || cameraXrotacc + angle < -75)
      return;
    cameraXRotation = RotateX(-angle) * cameraXRotation;
    cameraXrotacc += angle;
    break;
  case Ry:
    cameraYRotation = RotateY(-angle) * cameraYRotation;
    break;
  }
}

void translateCamera(GLdouble x, GLdouble y, GLdouble z) {
  cameraTranslate = Translate(-x, -y, -z) * cameraTranslate;
}

/******************************************************************/

model main_model;
vec4 *normals = nullptr;
GLfloat *selected = nullptr;
GLfloat *vindex = nullptr;
vec4 *UVs = nullptr;
vec4 *vertices = nullptr;
GLint len_verts = 0;

#define MODEL_EMPTY (main_model.is_empty())
#define MODEL_NUM_VERTICES (main_model.triangles.size() * 3)

GLuint modelVao;
GLuint modelBuffer;
GLuint modelShader;
GLuint editEdgesShader;
GLuint modelTransform;
GLuint modelNormal;
GLuint modelVSelected;
GLuint modelVindex;
GLuint modelVPosition;
GLuint modelDiffuseProperty;
GLuint modelSpecularProperty;
GLuint modelAmbientProperty;
GLuint modelShininess;
GLuint modelIsFakeDisplay;
GLuint modelIsEdge;
GLuint lightPosition;
GLuint modelHasTexture;
GLuint modelTexture;
GLuint modelUVPoint;

namespace uvw {
GLuint vPosition;
GLuint vSelected;
GLuint uSelected;
GLuint uniqueColors;
GLuint vindex;
GLuint Projection;
GLuint Transform;
GLuint isFakeDisplay;

GLuint VAO;
GLuint VBO;

GLuint Program;
GLfloat *selected;
} // namespace uvw

namespace uvTexture {
GLuint textureLocation;

GLuint texture;

GLuint VAO;
GLuint VBO;
GLuint vPosition;
GLuint Projection;

GLuint Program;

// 2d square
vec4 vertices[6] = {vec4(-1.0, -1.0, 0.0, 1.0), vec4(1.0, -1.0, 0.0, 1.0),
                    vec4(1.0, 1.0, 0.0, 1.0),   vec4(-1.0, -1.0, 0.0, 1.0),
                    vec4(1.0, 1.0, 0.0, 1.0),   vec4(-1.0, 1.0, 0.0, 1.0)};

} // namespace uvTexture

/******************************************************************/

void init_buffers() {
  // there is only one cube stored as vao

  glGenVertexArrays(1, &uvTexture::VAO);
  glBindVertexArray(uvTexture::VAO);

  glGenBuffers(1, &uvTexture::VBO);
  glBindBuffer(GL_ARRAY_BUFFER, uvTexture::VBO);

  glGenVertexArrays(1, &modelVao);
  glBindVertexArray(modelVao);

  glGenBuffers(1, &modelBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, modelBuffer);

  glGenVertexArrays(1, &uvw::VAO);
  glBindVertexArray(uvw::VAO);

  glGenBuffers(1, &uvw::VBO);
  glBindBuffer(GL_ARRAY_BUFFER, uvw::VBO);
}

void setup_texture() {
  auto texture_path = main_model.material.texture_path;
  if (texture_path == "") {
    return;
  }

  int width, height;
  uvTexture::textureLocation = loadPNG(texture_path.c_str(), &width, &height);

  std::cout << width << " " << height << std::endl;

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, uvTexture::textureLocation);
  glUniform1i(uvTexture::texture, 0);
}

void setup_uv_image_program() {
  uvTexture::Program = InitShader("uv_vimage.glsl", "uv_fimage.glsl");
  uvTexture::vPosition = glGetAttribLocation(uvTexture::Program, "vPosition");
  uvTexture::texture = glGetUniformLocation(uvTexture::Program, "texture1");
  uvTexture::Projection =
      glGetUniformLocation(uvTexture::Program, "Projection");
  glUseProgram(uvTexture::Program);
  glBindVertexArray(uvTexture::VAO);
  glBindBuffer(GL_ARRAY_BUFFER, uvTexture::VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vec4) * 6, uvTexture::vertices,
               GL_STATIC_DRAW);
  glEnableVertexAttribArray(uvTexture::vPosition);
  glVertexAttribPointer(uvTexture::vPosition, 4, GL_FLOAT, GL_FALSE, 0,
                        BUFFER_OFFSET(0));

  setup_texture();
}

//----------------------------------------------------------------------------
void init(void) {
  SETUP_SWIND_LINE_SHADER();
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);

  // Load shaders and use the resulting shader program
  modelShader = InitShader("vshader.glsl", "fshader.glsl");
  uvw::Program = InitShader("uv_vshader.glsl", "uv_fshader.glsl");
  init_buffers();
  setup_uv_image_program();

  projection = // Ortho(-4, 4, -4, 4, -100, 100);
      on_get_leftsize_ortho(500, 500, &object_window) *
      Perspective(90, 1.0, 0.1, 200);

  glUseProgram(uvw::Program);

  uvw::vPosition = glGetAttribLocation(uvw::Program, "vPosition");
  uvw::vSelected = glGetAttribLocation(uvw::Program, "vSelected");
  uvw::uSelected = glGetAttribLocation(uvw::Program, "uSelected");
  uvw::vindex = glGetAttribLocation(uvw::Program, "vindex");
  uvw::Projection = glGetUniformLocation(uvw::Program, "Projection");
  uvw::Transform = glGetUniformLocation(uvw::Program, "Transform");
  uvw::isFakeDisplay = glGetUniformLocation(uvw::Program, "isFakeDisplay");
  uvw::uniqueColors = glGetAttribLocation(uvw::Program, "uniqueColors");

  glVertexAttribPointer(uvw::vPosition, 4, GL_FLOAT, GL_FALSE, 0,
                        BUFFER_OFFSET(0));

  glUseProgram(modelShader);

  modelTransform = glGetUniformLocation(modelShader, "Transform");
  cameraProjection = glGetUniformLocation(modelShader, "Projection");
  modelVPosition = glGetAttribLocation(modelShader, "vPosition");
  modelUVPoint = glGetAttribLocation(modelShader, "UVpos");
  modelVSelected = glGetAttribLocation(modelShader, "vSelected");
  modelVindex = glGetAttribLocation(modelShader, "vindex");
  modelNormal = glGetAttribLocation(modelShader, "vNormal");
  modelDiffuseProperty = glGetUniformLocation(modelShader, "diffuseProperty");
  modelSpecularProperty = glGetUniformLocation(modelShader, "specularProperty");
  modelAmbientProperty = glGetUniformLocation(modelShader, "ambientProperty");
  modelShininess = glGetUniformLocation(modelShader, "shininess");
  modelIsFakeDisplay = glGetUniformLocation(modelShader, "isFakeDisplay");
  modelHasTexture = glGetUniformLocation(modelShader, "hasTexture");
  modelTexture = glGetUniformLocation(modelShader, "texture1");
  modelIsEdge = glGetUniformLocation(modelShader, "isEdge");
  lightPosition = glGetUniformLocation(modelShader, "lightPosition");

  glVertexAttribPointer(modelVPosition, 4, GL_FLOAT, GL_FALSE, 0,
                        BUFFER_OFFSET(0));

  glClearColor(1.0, 1.0, 1.0, 1.0); // white background
  glEnable(GL_DEPTH_TEST);
}

//----------------------------------------------------------------------------

void display_model(bool fake) {

  glUseProgram(modelShader);
  auto ci = getCameraInverse();
  glBindVertexArray(modelVao);
  glBindBuffer(GL_ARRAY_BUFFER, modelBuffer);
  glBufferData(GL_ARRAY_BUFFER,
               (sizeof(vec4) * len_verts * 3) +
                   (sizeof(GLfloat) * len_verts * 2),
               NULL, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec4) * len_verts, vertices);
  if (!fake) {
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec4) * len_verts,
                    sizeof(vec4) * len_verts, normals);
  } else {
    // if fake, instead of normals, put unique colors
    vec4 *ucolors = (vec4 *)calloc(len_verts, sizeof(vec4));
    for (int i = 0; i < len_verts / 3; i++) {
      ucolors[i * 3] = INT_TO_UNIQUE_COLOR(i);
      ucolors[(i * 3) + 1] = INT_TO_UNIQUE_COLOR(i);
      ucolors[(i * 3) + 2] = INT_TO_UNIQUE_COLOR(i);
    }
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec4) * len_verts,
                    sizeof(vec4) * len_verts, ucolors);
    free(ucolors);
  }
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec4) * len_verts * 2,
                  sizeof(GLfloat) * len_verts, selected);
  glBufferSubData(GL_ARRAY_BUFFER,
                  (sizeof(vec4) * len_verts * 2) +
                      (sizeof(GLfloat) * len_verts),
                  sizeof(GLfloat) * len_verts, vindex);
  glBufferSubData(GL_ARRAY_BUFFER,
                  (sizeof(vec4) * len_verts * 2) +
                      (sizeof(GLfloat) * 2 * len_verts),
                  sizeof(vec4) * len_verts, UVs);
  glUniformMatrix4fv(modelTransform, 1, GL_TRUE, ci /* * RotateX(-90)*/);
  glUniformMatrix4fv(cameraTransform, 1, GL_TRUE, projection);
  glUniform4fv(modelDiffuseProperty, 1, main_model.material.diffuse);
  glUniform4fv(modelSpecularProperty, 1, main_model.material.specular);
  glUniform4fv(modelAmbientProperty, 1, main_model.material.ambient);
  auto light = vec4(0.0, 2.0, 0.0, 0.0);
  glUniform4fv(lightPosition, 1, ci * light);
  glUniform1f(modelShininess, main_model.material.shininess);
  if (!fake) {
    glUniform1f(modelIsFakeDisplay, 0);
    glUniform1i(modelIsEdge, 1);
    glUniform1i(modelHasTexture,
                main_model.material.texture_path != "" ? 1 : 0);
  } else {
    glUniform1f(modelIsFakeDisplay, 1);
    glUniform1i(modelIsEdge, 0);
    glUniform1i(modelHasTexture, 0);
  }
  glBindTexture(GL_TEXTURE_2D, uvTexture::textureLocation);
  glUniform1i(modelTexture, 0);
  glEnableVertexAttribArray(modelVPosition);
  glVertexAttribPointer(modelVPosition, 4, GL_FLOAT, GL_FALSE, 0,
                        BUFFER_OFFSET(0));
  glEnableVertexAttribArray(modelNormal);
  glVertexAttribPointer(modelNormal, 4, GL_FLOAT, GL_FALSE, 0,
                        BUFFER_OFFSET(sizeof(vec4) * len_verts));
  glEnableVertexAttribArray(modelVSelected);
  glVertexAttribPointer(modelVSelected, 1, GL_FLOAT, GL_FALSE, 0,
                        BUFFER_OFFSET(sizeof(vec4) * len_verts * 2));
  glEnableVertexAttribArray(modelVindex);
  glVertexAttribPointer(modelVindex, 1, GL_FLOAT, GL_FALSE, 0,
                        BUFFER_OFFSET((sizeof(vec4) * len_verts * 2) +
                                      (sizeof(GLfloat) * len_verts)));
  glEnableVertexAttribArray(modelUVPoint);
  glVertexAttribPointer(modelUVPoint, 4, GL_FLOAT, GL_FALSE, 0,
                        BUFFER_OFFSET((sizeof(vec4) * len_verts * 2) +
                                      (sizeof(GLfloat) * 2 * len_verts)));
  glViewport(object_window.get_x_offset(), object_window.get_y_offset(),
             object_window.get_absolute_width(),
             object_window.get_absolute_height());
  glDrawArrays(GL_TRIANGLES, 0, MODEL_NUM_VERTICES);
}

mat4 getUvProjection() {
  auto width = uv_window.get_absolute_width();
  auto height = uv_window.get_absolute_height();
  float aspectRatio = ((float)width) / ((float)height);
  /*
  Orthographic projection such that the smallest side is 1.0
  */
  if (aspectRatio < 1) {
    // print all arguments to ortho
    return Ortho(-1, 1, -1.0 / aspectRatio, 1.0 / aspectRatio, -5, 5);
  } else {
    return Ortho(-1 * aspectRatio, 1.0 * aspectRatio, -1, 1, -5, 5);
  }
}

void display_uv_edit(bool fake) {

  glUseProgram(uvw::Program);
  glBindVertexArray(uvw::VAO);
  glBindBuffer(GL_ARRAY_BUFFER, uvw::VBO);
  // buffer organization:
  /*
  1. verts
  2. unique colors
  3. selected
  4. uselected
  5. vindex
  */
  glBufferData(GL_ARRAY_BUFFER,
               (sizeof(vec4) * len_verts) + (sizeof(vec4) * len_verts) +
                   (sizeof(GLfloat) * 3 * len_verts),
               NULL, GL_STATIC_DRAW);

  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec4) * len_verts, UVs);

  if (!fake) {
  } else {
    // if fake,  put unique colors
    vec4 *ucolors = (vec4 *)calloc(len_verts, sizeof(vec4));
    for (int i = 0; i < len_verts / 3; i++) {
      ucolors[i * 3] = INT_TO_UNIQUE_COLOR(i);
      ucolors[(i * 3) + 1] = INT_TO_UNIQUE_COLOR(i);
      ucolors[(i * 3) + 2] = INT_TO_UNIQUE_COLOR(i);
    }
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec4) * len_verts,
                    sizeof(vec4) * len_verts, ucolors);
    free(ucolors);
  }
  glBufferSubData(GL_ARRAY_BUFFER,
                  (sizeof(vec4) * len_verts) + (sizeof(vec4) * len_verts),
                  sizeof(GLfloat) * len_verts, selected);
  glBufferSubData(GL_ARRAY_BUFFER,
                  (sizeof(vec4) * len_verts) + (sizeof(vec4) * len_verts) +
                      (sizeof(GLfloat) * len_verts),
                  sizeof(GLfloat) * len_verts, uvw::selected);
  glBufferSubData(GL_ARRAY_BUFFER,
                  (sizeof(vec4) * len_verts) + (sizeof(vec4) * len_verts) +
                      (sizeof(GLfloat) * 2 * len_verts),
                  sizeof(GLfloat) * len_verts, vindex);

  glUniformMatrix4fv(uvw::Transform, 1, GL_TRUE, mat4());
  glUniformMatrix4fv(uvw::Projection, 1, GL_TRUE, getUvProjection());
  if (!fake) {
    glUniform1f(uvw::isFakeDisplay, 0);
  } else {
    glUniform1f(uvw::isFakeDisplay, 1);
  }
  glEnableVertexAttribArray(uvw::vPosition);
  glVertexAttribPointer(uvw::vPosition, 4, GL_FLOAT, GL_FALSE, 0,
                        BUFFER_OFFSET(0));
  glEnableVertexAttribArray(uvw::uniqueColors);
  glVertexAttribPointer(uvw::uniqueColors, 4, GL_FLOAT, GL_FALSE, 0,
                        BUFFER_OFFSET(sizeof(vec4) * len_verts));
  glEnableVertexAttribArray(uvw::vSelected);
  glVertexAttribPointer(
      uvw::vSelected, 1, GL_FLOAT, GL_FALSE, 0,
      BUFFER_OFFSET((sizeof(vec4) * len_verts) + (sizeof(vec4) * len_verts)));
  glEnableVertexAttribArray(uvw::uSelected);
  glVertexAttribPointer(uvw::uSelected, 1, GL_FLOAT, GL_FALSE, 0,
                        BUFFER_OFFSET((sizeof(vec4) * len_verts) +
                                      (sizeof(vec4) * len_verts) +
                                      (sizeof(GLfloat) * len_verts)));
  glEnableVertexAttribArray(uvw::vindex);
  glVertexAttribPointer(uvw::vindex, 1, GL_FLOAT, GL_FALSE, 0,
                        BUFFER_OFFSET((sizeof(vec4) * len_verts) +
                                      (sizeof(vec4) * len_verts) +
                                      (sizeof(GLfloat) * len_verts * 2)));
  glViewport(uv_window.get_x_offset(), uv_window.get_y_offset(),
             uv_window.get_absolute_width(), uv_window.get_absolute_height());
  glDrawArrays(GL_TRIANGLES, 0, MODEL_NUM_VERTICES);
}

void display_uv_image_square(bool fake) {
  if (fake) {
    return;
  }
  glUseProgram(uvTexture::Program);
  glBindVertexArray(uvTexture::VAO);
  glBindBuffer(GL_ARRAY_BUFFER, uvTexture::VBO);
  glUniformMatrix4fv(uvTexture::Projection, 1, GL_TRUE, getUvProjection());
  glUniform1i(uvTexture::texture, 0);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vec4) * 6, uvTexture::vertices,
               GL_STATIC_DRAW);
  glEnableVertexAttribArray(uvTexture::vPosition);
  glVertexAttribPointer(uvTexture::vPosition, 4, GL_FLOAT, GL_FALSE, 0,
                        BUFFER_OFFSET(0));
  glViewport(uv_window.get_x_offset(), uv_window.get_y_offset(),
             uv_window.get_absolute_width(), uv_window.get_absolute_height());
  glDrawArrays(GL_TRIANGLES, 0, 6);
}

void display(bool fake) {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the buffer
  glEnable(GL_CULL_FACE);

  if (!MODEL_EMPTY) {
    display_model(fake);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    display_uv_image_square(fake);
    display_uv_edit(fake);
    glDisable(GL_BLEND);
  }
  glFlush();
  glViewport(0, 0, _width, _height);
  if (!fake) {
    glClear(GL_DEPTH_BUFFER_BIT);
    object_window.draw_edges();
    uv_window.draw_edges();
    glFlush();
  }
}

void update(void) {}

//----------------------------------------------------------------------------
void object_key_callback(GLFWwindow *window, int key, int scancode, int action,
                         int mods) {
  switch (key) {

  case GLFW_KEY_W:
    translateCamera(0.0, 0.0, -0.05);
    break;
  // move around with wasd, go up down with q e, look left right with n m
  case GLFW_KEY_S:
    translateCamera(0.0, 0.0, 0.05);
    break;
  case GLFW_KEY_A:
    translateCamera(-0.05, 0.0, 0.0);
    break;
  case GLFW_KEY_D:
    translateCamera(0.05, 0.0, 0.0);
    break;
  case GLFW_KEY_Q:
    translateCamera(0.0, -0.05, 0.0);
    break;
  case GLFW_KEY_E:
    translateCamera(0.0, 0.05, 0.0);
    break;
  case GLFW_KEY_N:
    rotateCamera(Ry, -.5);
    break;
  case GLFW_KEY_M:
    rotateCamera(Ry, .5);
    break;
  // rotate up and down by j k
  case GLFW_KEY_J:
    rotateCamera(Rx, -.5);
    break;
  case GLFW_KEY_K:
    rotateCamera(Rx, .5);
    break;
  }
}

vec4 getFaceUVCenter(int index) {
  int istart = index / 3;
  vec4 center = vec4(0.0, 0.0, 0.0, 0.0);
  for (int i = 0; i < 3; i++) {
    center = center + UVs[(istart * 3) + i];
  }
  return center / 3.0;
}

vec4 getUVSelectedCenter() {
  vec4 center = vec4(0.0, 0.0, 0.0, 0.0);
  int count = 0;
  for (int i = 0; i < MODEL_NUM_VERTICES; i++) {
    if (uvw::selected[i] == 1.0) {
      center = center + UVs[i];
      count++;
    }
  }
  return center / count;
}

void uv_key_callback(GLFWwindow *window, int key, int scancode, int action,
                     int mods) {
  /**
   * Controls:
   Move Selected: WASD
   Scale Selected: x: HJ y: NM
   Rotate Selected: QE
   *
   */

  switch (key) {
  case GLFW_KEY_W:
    for (int i = 0; i < MODEL_NUM_VERTICES; i++) {
      if (uvw::selected[i] == 1.0) {
        UVs[i] = Translate(0.0, 0.05, 0.0) * UVs[i];
        *(main_model.triangles[i / 3].uget(i % 3)) = vec2(UVs[i].x, UVs[i].y);
      }
    }
    break;

  case GLFW_KEY_S:
    for (int i = 0; i < MODEL_NUM_VERTICES; i++) {
      if (uvw::selected[i] == 1.0) {
        UVs[i] = Translate(0.0, -0.05, 0.0) * UVs[i];
        *(main_model.triangles[i / 3].uget(i % 3)) = vec2(UVs[i].x, UVs[i].y);
      }
    }
    break;

  case GLFW_KEY_A:
    for (int i = 0; i < MODEL_NUM_VERTICES; i++) {
      if (uvw::selected[i] == 1.0) {
        UVs[i] = Translate(-0.05, 0.0, 0.0) * UVs[i];
        *(main_model.triangles[i / 3].uget(i % 3)) = vec2(UVs[i].x, UVs[i].y);
      }
    }
    break;

  case GLFW_KEY_D:
    for (int i = 0; i < MODEL_NUM_VERTICES; i++) {
      if (uvw::selected[i] == 1.0) {
        UVs[i] = Translate(0.05, 0.0, 0.0) * UVs[i];
        *(main_model.triangles[i / 3].uget(i % 3)) = vec2(UVs[i].x, UVs[i].y);
      }
    }
    break;

  case GLFW_KEY_H:
    for (int i = 0; i < MODEL_NUM_VERTICES; i++) {
      if (uvw::selected[i] == 1.0) {
        vec4 faceCenter = getUVSelectedCenter();
        UVs[i] = Translate(faceCenter) * Scale(1.0, .95, 1.0) *
                 Translate(-faceCenter) * UVs[i];
        *(main_model.triangles[i / 3].uget(i % 3)) = vec2(UVs[i].x, UVs[i].y);
      }
    }
    break;

  case GLFW_KEY_J:
    for (int i = 0; i < MODEL_NUM_VERTICES; i++) {
      if (uvw::selected[i] == 1.0) {
        vec4 faceCenter = getUVSelectedCenter();
        UVs[i] = Translate(faceCenter) * Scale(1.0, 1.05, 1.0) *
                 Translate(-faceCenter) * UVs[i];
        *(main_model.triangles[i / 3].uget(i % 3)) = vec2(UVs[i].x, UVs[i].y);
      }
    }
    break;

  case GLFW_KEY_N:
    for (int i = 0; i < MODEL_NUM_VERTICES; i++) {
      if (uvw::selected[i] == 1.0) {
        vec4 faceCenter = getUVSelectedCenter();
        UVs[i] = Translate(faceCenter) * Scale(.95, 1.0, 1.0) *
                 Translate(-faceCenter) * UVs[i];
        *(main_model.triangles[i / 3].uget(i % 3)) = vec2(UVs[i].x, UVs[i].y);
      }
    }
    break;

  case GLFW_KEY_M:
    for (int i = 0; i < MODEL_NUM_VERTICES; i++) {
      if (uvw::selected[i] == 1.0) {
        vec4 faceCenter = getUVSelectedCenter();
        UVs[i] = Translate(faceCenter) * Scale(1.05, 1.0, 1.0) *
                 Translate(-faceCenter) * UVs[i];
        *(main_model.triangles[i / 3].uget(i % 3)) = vec2(UVs[i].x, UVs[i].y);
      }
    }
    break;

  case GLFW_KEY_Q:
    for (int i = 0; i < MODEL_NUM_VERTICES; i++) {
      if (uvw::selected[i] == 1.0) {
        vec4 faceCenter = getUVSelectedCenter();
        UVs[i] = Translate(faceCenter) * RotateZ(2) * Translate(-faceCenter) *
                 UVs[i];
        *(main_model.triangles[i / 3].uget(i % 3)) = vec2(UVs[i].x, UVs[i].y);
      }
    }
    break;

  case GLFW_KEY_E:
    for (int i = 0; i < MODEL_NUM_VERTICES; i++) {
      if (uvw::selected[i] == 1.0) {
        vec4 faceCenter = getUVSelectedCenter();
        UVs[i] = Translate(faceCenter) * RotateZ(-2) * Translate(-faceCenter) *
                 UVs[i];
        *(main_model.triangles[i / 3].uget(i % 3)) = vec2(UVs[i].x, UVs[i].y);
      }
    }
    break;
  }
}

// exit when escape key pressed
void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods) {
  switch (key) {
  case GLFW_KEY_ESCAPE:
    exit(EXIT_SUCCESS);
    break;
  }
  switch (SelectedWindow::selected) {
  case SelectedWindow::OBJECT:
    object_key_callback(window, key, scancode, action, mods);
    break;
  case SelectedWindow::UV:
    uv_key_callback(window, key, scancode, action, mods);
    break;
  }
}

GLfloat dotp(vec4 a, vec4 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
  // get window size
  int width, height;
  glfwGetWindowSize(window, &width, &height);
  // get cursor position
  double xpos, ypos;
  glfwGetCursorPos(window, &xpos, &ypos);

  if (!object_window.is_point_in(xpos, ypos)) {
    return;
  }

  if (cameraDistance + yoffset * 0.2 > 1.2 &&
      cameraDistance + yoffset * 0.2 < 6.5) {
    translateCamera(0.0, 0.0, yoffset * 0.2);
    cameraDistance += yoffset * 0.2;
  }
}
void mouse_key_callback(GLFWwindow *window, int button, int action, int mods) {
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    if (!(object_window.is_point_in(xpos, ypos) ||
          uv_window.is_point_in(xpos, ypos))) {
      SelectedWindow::selected = -1;
      return;
    }

    if (object_window.is_point_in(xpos, ypos)) {
      SelectedWindow::selected = SelectedWindow::OBJECT;
    } else {
      SelectedWindow::selected = SelectedWindow::UV;
    }

    // draw with unique colors
    display(true);

    unsigned char data[4];
    glReadPixels(xpos, height - ypos, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
    vec4 color = vec4(data[0] / 255.0, data[1] / 255.0, data[2] / 255.0, 1.0);
    int index = UNIQUE_COLOR_TO_INT(color);
    if (object_window.is_point_in(xpos, ypos)) {
      if (index < MODEL_NUM_VERTICES) {
        selected[index * 3] = selected[index * 3] == 0.0 ? 1.0 : 0.0;
        selected[(index * 3) + 1] =
            selected[(index * 3) + 1] == 0.0 ? 1.0 : 0.0;
        selected[(index * 3) + 2] =
            selected[(index * 3) + 2] == 0.0 ? 1.0 : 0.0;

        uvw::selected[index * 3] = 0.0;
        uvw::selected[(index * 3) + 1] = 0.0;
        uvw::selected[(index * 3) + 2] = 0.0;
      }
    } else {
      std::cout << "UV WINDOW " << color << " " << index << std::endl;
      if (index < MODEL_NUM_VERTICES) {
        uvw::selected[index * 3] = uvw::selected[index * 3] == 0.0 ? 1.0 : 0.0;
        uvw::selected[(index * 3) + 1] =
            uvw::selected[(index * 3) + 1] == 0.0 ? 1.0 : 0.0;
        uvw::selected[(index * 3) + 2] =
            uvw::selected[(index * 3) + 2] == 0.0 ? 1.0 : 0.0;
      }
    }
  }
}

bool _close_enough(vec4 a, vec4 b) {
  // std::cout << "close_enough " << a << " " << b << "   ";
  GLfloat dp = ((a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w));
  GLfloat alen = ((a.x * a.x) + (a.y * a.y) + (a.z * a.z) + (a.w * a.w));

  return std::abs(std::abs(dp) - std::abs(alen)) < 1e-3;
}

void cursor_pos_callback(GLFWwindow *window, double xpos, double ypos) {
  /*
  if (holding_down) {
    GLfloat dx = xpos - mouse_initial_click_pos.x;
    GLfloat dy = ypos - mouse_initial_click_pos.y;

    vec2 drag_d = vec2(dx, -dy);
    if (!camera_rotating) {
      if (!isRotating) {
        mouse_initial_click_pos = vec2(xpos, ypos);

        uid face_id = selected_face - 1;
        uid part_id = face_id / 6;
        GLuint face_initial_orientation = face_id % 6;
        if (length(drag_d) < 10)
          return;
        drag_d = drag_d / length(drag_d);

        vec4 face_normal = normal_from_face(face_initial_orientation);
        vec4 rotated_face_normal =
            selected_piece->rotationTransform * face_normal;

        std::cout << "ROTATED_FACE_NORMAL " << rotated_face_normal << std::endl;

        auto ci = getCameraInverse();

        std::vector<mappedRa> pairs = std::vector<mappedRa>();

        for (int i = 0; i < 6; i++) {
          vec4 projected_normal = projection * ci * cubeNormals[i].normal;
          std::cout << "CN, PN, D " << cubeNormals[i].normal << "  "
                    << projected_normal << "  " << drag_d << std::endl;
          vec2 trimmed = vec2(projected_normal.x, projected_normal.y);
          auto pair =
              mappedRa(((trimmed.x * drag_d.x) + (trimmed.y * drag_d.y)),
                       cubeNormals[i]);
          pairs.push_back(pair);
        }

        // sort
        std::sort(pairs.begin(), pairs.end(), raCompare);

        std::cout << "SORTED: " << std::endl;

        for (auto i : pairs) {
          std::cout << i.first << " : " << AXIS_STR(i.second.axis)
                    << i.second.normal << i.second.direction << std::endl;
        }

        std::cout << "----" << std::endl;

        // pairs max contain the direction
        // apairs min contain the axis

        mappedRa pairs_max;
        mappedRa apairs_min;

        for (auto i : pairs) {
          if (!_close_enough(i.second.normal, rotated_face_normal)) {
            pairs_max = i;
            break;
          }
        }


        vec3 cprod = cross(rotated_face_normal, pairs_max.second.normal);
        std::cout << rotated_face_normal << " " << pairs_max.second.normal
                  << " "
                  << "CROSS PRODUCT: " << cprod << std::endl;

        // for (auto i : apairs) {
        //   if (!_close_enough(i.second.normal, rotated_face_normal)) {
        //     apairs_min = i;
        //     break;
        //   }
        // }

        if (std::abs(std::abs(cprod.x) - 1) < 1e-2) {
          rotationAxis = Rx;
          rotationDirection = std::signbit(cprod.x) == 0 ? 1 : -1;
        }
        if (std::abs(std::abs(cprod.y) - 1) < 1e-2) {
          rotationAxis = Ry;
          rotationDirection = std::signbit(cprod.y) == 0 ? 1 : -1;
        }
        if (std::abs(std::abs(cprod.z) - 1) < 1e-2) {
          rotationAxis = Rz;
          rotationDirection = std::signbit(cprod.z) == 0 ? 1 : -1;
        }

        if (true) {
          // rotationAxis = apairs_min.second.axis;

          isRotating = GL_TRUE;
          startingPosition = selected_piece->position[rotationAxis];

          std::cout << "Rotating around " << rotationAxis << " with direction "
                    << rotationDirection << " starting from "
                    << startingPosition << std::endl;
        }

        // isRotating = GL_TRUE;
        // startingPosition = selected_piece->position[rotationAxis];

        // // print all rotation info
        // std::cout << "Rotating around " << rotationAxis << " with direction "
        //           << rotationDirection << " starting from " <<
        //           startingPosition
        //           << std::endl;
      }
    } else {
      mouse_initial_click_pos = vec2(xpos, ypos);

      if (!limited_camera) {
        cameraXRotation =
            RotateY(dx * 0.4) * RotateX(dy * 0.4) * cameraXRotation;
      } else {
        rotateCamera(Rx, -dy * 0.4);
        rotateCamera(Ry, -dx * 0.4);
      }
    }
  }*/
}
//----------------------------------------------------------------------------
// handle window resize

/**
 * @brief project the view so that right side of the screen is empty
 *
 * @param width
 * @param height
 */
mat4 on_get_leftsize_ortho(GLint width, GLint height, subwindow *window) {
  auto m = window->get_transform_matrix();
  // std::cout << "TRANSFORM MATRIX: " << m << std::endl;
  // m[0][3] = 0;
  // return m;
  return identity();
}

void on_window_resize(GLFWwindow *window, GLint width, GLint height) {
  main_window.height.regular.value = height;
  main_window.width.regular.value = width;

  GLfloat aspectRatio =
      object_window.get_absolute_width() / object_window.get_absolute_height();
  // std::cout << width << " " << height << " " << aspectRatio << std::endl;
  // std::cout << object_window.get_absolute_width() << " "
  //           << object_window.get_absolute_height() << " "
  //           << object_window.get_x_offset() << " "
  //           << object_window.get_y_offset() << std::endl;
  GLfloat FoV;
  if (aspectRatio < 1) {
    // GPT generated dynamic FoV line
    FoV =
        2 * atan(tan(90 * (M_PI / 180) / 2) * (1 / aspectRatio)) * (180 / M_PI);
  } else {
    FoV = 90.0;
  }
  projection = on_get_leftsize_ortho(width, height, &object_window) *
               Perspective(FoV, aspectRatio, 0.01, 100);
  // glViewport(0, 0, width, height);
  _width = width;
  _height = height;
}

//----------------------------------------------------------------------------
float diffuse_chooser[3];
float specular_chooser[3];

void load_verts(model *my_model) {
  renormalizeUVs(my_model);
  to_point_array(&vertices, &len_verts, &my_model->triangles);
  vec2 *tmp;
  to_point_array_uv(&tmp, &my_model->triangles);
  UVs = (vec4 *)malloc(sizeof(vec4) * len_verts);
  for (int i = 0; i < len_verts; i++) {
    UVs[i] = vec4(tmp[i].x, tmp[i].y, 2.0, 1.0);
  }
  free(tmp);
}

void load_model_in(std::string filename) {
  std::ifstream file2(filename);
  std::stringstream buffer;
  buffer << file2.rdbuf();
  std::string json2 = buffer.str();
  file2.close();

  serializable_model smodel;
  smodel.zax_from_json(json2.c_str());

  std::cout << "Loaded Model " << smodel << std::endl;

  main_model = to_model(smodel);

  if (normals != nullptr) {
    free(normals);
  }
  normals = (vec4 *)malloc(main_model.triangles.size() * 3 * sizeof(vec4));
  generate_normals(normals, &main_model, true);

  if (selected != nullptr) {
    free(selected);
  }
  if (uvw::selected != nullptr) {
    free(uvw::selected);
  }
  selected =
      (GLfloat *)malloc(main_model.triangles.size() * 3 * sizeof(GLfloat));

  uvw::selected =
      (GLfloat *)malloc(main_model.triangles.size() * 3 * sizeof(GLfloat));

  if (UVs != nullptr) {
    free(UVs);
  }

  if (vertices != nullptr) {
    free(vertices);
  }

  load_verts(&main_model);

  for (int i = 0; i < main_model.triangles.size() * 3; i++) {
    selected[i] = 0.0;
    uvw::selected[i] = 0.0;
  }

  if (vindex != nullptr) {
    free(vindex);
  }
  vindex = (GLfloat *)malloc(main_model.triangles.size() * 3 * sizeof(GLfloat));

  for (int i = 0; i < main_model.triangles.size() * 3; i++) {
    vindex[i] = i;
  }

  diffuse_chooser[0] = main_model.material.diffuse.x;
  diffuse_chooser[1] = main_model.material.diffuse.y;
  diffuse_chooser[2] = main_model.material.diffuse.z;

  specular_chooser[0] = main_model.material.specular.x;
  specular_chooser[1] = main_model.material.specular.y;
  specular_chooser[2] = main_model.material.specular.z;

  setup_texture();
}

void recenterSingleUV(triangle *t) {
  vec2 uvcenter = (t->uv0 + t->uv1 + t->uv2) / 3.0;
  while (uvcenter.x < 0) {
    uvcenter.x += 1;
    t->uv0.x += 1;
    t->uv1.x += 1;
    t->uv2.x += 1;
  }
  while (uvcenter.x > 1) {
    uvcenter.x -= 1;
    t->uv0.x -= 1;
    t->uv1.x -= 1;
    t->uv2.x -= 1;
  }
  while (uvcenter.y < 0) {
    uvcenter.y += 1;
    t->uv0.y += 1;
    t->uv1.y += 1;
    t->uv2.y += 1;
  }
  while (uvcenter.y > 1) {
    uvcenter.y -= 1;
    t->uv0.y -= 1;
    t->uv1.y -= 1;
    t->uv2.y -= 1;
  }
}

void recenterSelectedUVs() {

  for (int i = 0; i < main_model.triangles.size() * 3; i++) {
    if (selected[i] == 1.0) {
      int face = i / 3;
      triangle *t = &main_model.triangles[face];
      recenterSingleUV(t);
    }
  }
  if (UVs != nullptr) {
    free(UVs);
  }

  if (vertices != nullptr) {
    free(vertices);
  }

  load_verts(&main_model);
}

void invert_model_normals() {
  invert_normals(&main_model);
  generate_normals(normals, &main_model, true);
  if (UVs != nullptr) {
    free(UVs);
  }

  if (vertices != nullptr) {
    free(vertices);
  }

  load_verts(&main_model);
}

//----------------------------------------------------------------------------

int main(int argc, char **argv) {
  if (!glfwInit())
    exit(EXIT_FAILURE);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  GLFWwindow *window = glfwCreateWindow(500, 500, "Rubix Cube", NULL, NULL);
  glfwMakeContextCurrent(window);

  // comment out the following two lines for windows and linux (don't need glew
  // in macos)

#if defined(__linux__) || defined(_WIN32)
  glewExperimental = GL_TRUE;
  glewInit();
#elif defined(__APPLE__)
  // Code for macOS
#endif

  if (!window) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwSetWindowSizeLimits(window, 400, 400, GLFW_DONT_CARE, GLFW_DONT_CARE);

  glfwSetKeyCallback(window, key_callback);
  glfwSetMouseButtonCallback(window, mouse_key_callback);
  glfwSetCursorPosCallback(window, cursor_pos_callback);
  glfwSetScrollCallback(window, scroll_callback);
  glfwSetWindowSizeCallback(window, on_window_resize);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  // Setup Dear ImGui style
  ImGui::StyleColorsLight();

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);

  init();

  ImGui_ImplOpenGL3_Init();

  auto last_update_time = glfwGetTime();
  int numShuff = 0;
  while (!glfwWindowShouldClose(window)) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    auto current_time = glfwGetTime();
    // run at 24 fps
    if (current_time - last_update_time > 1.0 / 24.0) {
      last_update_time = current_time;
      update();
    }
    display(false);

    int width, height;
    glfwGetWindowSize(window, &width, &height);
    ImGui::SetNextWindowPos(ImVec2(simple_ui_window.get_x_offset(), 0));
    ImGui::SetNextWindowSize(
        ImVec2(simple_ui_window.get_absolute_width(), height));
    ImGui::Begin("Button Window", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings);

    ImGui::BeginChild("Load Model Box", ImVec2(-1, 100), true,
                      ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::TextWrapped("Model filename:");
    static char filename[256] = "";
    ImGui::InputText("File path:", filename, sizeof(filename));
    if (ImGui::Button("Load Model")) {
      load_model_in(filename);
    }
    ImGui::EndChild();

    if (!MODEL_EMPTY) {
      // shader settings
      ImGui::BeginChild("Model Properties Box", ImVec2(-1, 900), true,
                        ImGuiWindowFlags_HorizontalScrollbar);
      ImGui::TextWrapped("Model Properties:");

      ImGui::ColorPicker3("DiffuseColor", diffuse_chooser);
      main_model.material.diffuse =
          vec4(diffuse_chooser[0], diffuse_chooser[1], diffuse_chooser[2], 1.0);
      main_model.material.ambient = main_model.material.diffuse * 0.2;
      ImGui::ColorPicker3("SpecularColor", specular_chooser);
      main_model.material.specular = vec4(
          specular_chooser[0], specular_chooser[1], specular_chooser[2], 1.0);
      ImGui::SliderFloat("Shininess", &main_model.material.shininess, 0.0,
                         100.0);
      bool on_clicked = false;
      on_clicked = ImGui::Button("Invert Normals");

      if (on_clicked) {
        invert_model_normals();
      }

      if (ImGui::Button("Select All")) {
        for (int i = 0; i < main_model.triangles.size() * 3; i++) {
          selected[i] = 1.0;
        }
      }
      if (ImGui::Button("Deselect All")) {
        for (int i = 0; i < main_model.triangles.size() * 3; i++) {
          selected[i] = 0.0;
          uvw::selected[i] = 0.0;
        }
      }

      ImGui::TextWrapped("Save Model Path:");
      static char filename2[256] = "";
      ImGui::InputText("File path:", filename2, sizeof(filename2));
      if (ImGui::Button("Save Model")) {
        auto to_save = to_serializable_model(main_model);
        std::ofstream file(filename2);
        file << to_save.zax_to_json();
        file.close();
      }

      ImGui::EndChild();
      ImGui::BeginChild("Load Texture Box", ImVec2(-1, 100), true,
                        ImGuiWindowFlags_HorizontalScrollbar);
      ImGui::TextWrapped("Texture Path (absolute):");
      static char filename3[256] = "";
      ImGui::InputText("File path:", filename3, sizeof(filename3));
      if (ImGui::Button("Load Texture")) {
        main_model.material.texture_path = filename3;
        setup_texture();
      }
      ImGui::EndChild();
      ImGui::BeginChild("UV tools", ImVec2(-1, 200), true,
                        ImGuiWindowFlags_HorizontalScrollbar);
      if (ImGui::Button("Sphere Projection")) {
        sphericalProjection(&main_model, selected, normals);

        if (UVs != nullptr) {
          free(UVs);
        }
        if (vertices != nullptr) {
          free(vertices);
        }

        load_verts(&main_model);
      }

      if (ImGui::Button("Cylindrical Projection")) {
        cylindricalProjection(&main_model, selected);
        if (UVs != nullptr) {
          free(UVs);
        }
        if (vertices != nullptr) {
          free(vertices);
        }
        load_verts(&main_model);
      }

      if (ImGui::Button("Cube Projection")) {
        cubeProjection(&main_model, selected);
        if (UVs != nullptr) {
          free(UVs);
        }
        if (vertices != nullptr) {
          free(vertices);
        }
        load_verts(&main_model);
      }
      if (ImGui::Button("Project From Camera")) {
        projectFromView(&main_model, selected, projection * getCameraInverse());
        if (UVs != nullptr) {
          free(UVs);
        }
        if (vertices != nullptr) {
          free(vertices);
        }
        load_verts(&main_model);
      }
      if (ImGui::Button("UV Select All")) {
        for (int i = 0; i < main_model.triangles.size() * 3; i++) {
          if (selected[i] == 1.0)
            uvw::selected[i] = 1.0;
        }
      }
      if (ImGui::Button("UV Deselect All")) {
        for (int i = 0; i < main_model.triangles.size() * 3; i++) {
          uvw::selected[i] = 0.0;
        }
      }

      if (ImGui::Button("UV Center Selected")) {
        recenterSelectedUVs();
      }

      ImGui::EndChild();
    }

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
    glfwPollEvents(); // glfwWaitEvents
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  exit(EXIT_SUCCESS);
}
