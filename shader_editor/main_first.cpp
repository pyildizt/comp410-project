#include "data_types.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "load_model.hpp"
#include "mat.h"
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

/**
 * Camera Stuff
 *
 */

mat4 projection = identity();
mat4 cameraTranslate = Translate(0.0, 0.0, -2.0);
mat4 cameraXRotation = identity();
mat4 cameraYRotation = identity();
GLfloat cameraXrotacc = 0.0;

// We want camera to be able to rotate around the origin, with certain distance

mat4 getCameraInverse() {
  return cameraTranslate * cameraXRotation * cameraYRotation;
}

mat4 on_get_leftsize_ortho(GLint width, GLint height);

GLuint cameraProjection;
GLuint cameraTransform;
GLfloat cameraDistance = 2.0;

GLfloat UI_WIDTH = 200;

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
/******************************************************************/

void init_buffers() {
  // there is only one cube stored as vao

  glGenVertexArrays(1, &modelVao);
  glBindVertexArray(modelVao);

  glGenBuffers(1, &modelBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, modelBuffer);
}

//----------------------------------------------------------------------------
void init(void) {
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);

  // Load shaders and use the resulting shader program
  modelShader = InitShader("vshader.glsl", "fshader.glsl");
  init_buffers();

  projection = // Ortho(-4, 4, -4, 4, -100, 100);
      on_get_leftsize_ortho(500 + UI_WIDTH, 500) *
      Perspective(90, 1.0, 0.1, 200);

  glUseProgram(modelShader);

  modelTransform = glGetUniformLocation(modelShader, "Transform");
  cameraProjection = glGetUniformLocation(modelShader, "Projection");
  modelVPosition = glGetAttribLocation(modelShader, "vPosition");
  modelVSelected = glGetAttribLocation(modelShader, "vSelected");
  modelVindex = glGetAttribLocation(modelShader, "vindex");
  modelNormal = glGetAttribLocation(modelShader, "vNormal");
  modelDiffuseProperty = glGetUniformLocation(modelShader, "diffuseProperty");
  modelSpecularProperty = glGetUniformLocation(modelShader, "specularProperty");
  modelAmbientProperty = glGetUniformLocation(modelShader, "ambientProperty");
  modelShininess = glGetUniformLocation(modelShader, "shininess");
  modelIsFakeDisplay = glGetUniformLocation(modelShader, "isFakeDisplay");
  modelIsEdge = glGetUniformLocation(modelShader, "isEdge");
  lightPosition = glGetUniformLocation(modelShader, "lightPosition");

  glVertexAttribPointer(modelVPosition, 4, GL_FLOAT, GL_FALSE, 0,
                        BUFFER_OFFSET(0));

  glClearColor(1.0, 1.0, 1.0, 1.0); // white background
  glEnable(GL_DEPTH_TEST);
}

//----------------------------------------------------------------------------

void display(bool fake) {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the buffer
  glEnable(GL_CULL_FACE);

  if (!MODEL_EMPTY) {
    auto ci = getCameraInverse();
    glBindBuffer(GL_ARRAY_BUFFER, modelBuffer);
    vec4 *vertices;
    int len;
    to_point_array(&vertices, &len, &main_model.triangles);
    glBufferData(GL_ARRAY_BUFFER,
                 (sizeof(vec4) * len * 2) + (sizeof(GLfloat) * len * 2), NULL,
                 GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec4) * len, vertices);
    free(vertices);
    if(!fake){
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec4) * len, sizeof(vec4) * len,
                    normals);
    }else{
      // if fake, instead of normals, put unique colors
      vec4* ucolors = (vec4*)calloc(len,sizeof(vec4));
      for(int i = 0; i < len / 3; i++){
        ucolors[i*3] = INT_TO_UNIQUE_COLOR(i);
        ucolors[(i*3)+1] = INT_TO_UNIQUE_COLOR(i);
        ucolors[(i*3)+2] = INT_TO_UNIQUE_COLOR(i);
      }
      glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec4) * len, sizeof(vec4) * len,
                    ucolors);
      free(ucolors);
    }
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec4) * len * 2,
                    sizeof(GLfloat) * len, selected);
    glBufferSubData(GL_ARRAY_BUFFER,
                    (sizeof(vec4) * len * 2) + (sizeof(GLfloat) * len),
                    sizeof(GLfloat) * len, vindex);
    glUniformMatrix4fv(modelTransform, 1, GL_TRUE, ci * RotateX(-90));
    glUniformMatrix4fv(cameraTransform, 1, GL_TRUE, projection);
    glUniform4fv(modelDiffuseProperty, 1, main_model.material.diffuse);
    glUniform4fv(modelSpecularProperty, 1, main_model.material.specular);
    glUniform4fv(modelAmbientProperty, 1, main_model.material.ambient);
    auto light = vec4(0.0, 2.0, 0.0, 1.0);
    glUniform4fv(lightPosition, 1, ci * light);
    glUniform1f(modelShininess, main_model.material.shininess);
    if(!fake){
    glUniform1f(modelIsFakeDisplay, 0);
        glUniform1i(modelIsEdge, 1);
    }else{
      glUniform1f(modelIsFakeDisplay, 1);
          glUniform1i(modelIsEdge, 0);
}
    glEnableVertexAttribArray(modelVPosition);
    glVertexAttribPointer(modelVPosition, 4, GL_FLOAT, GL_FALSE, 0,
                          BUFFER_OFFSET(0));
    glEnableVertexAttribArray(modelNormal);
    glVertexAttribPointer(modelNormal, 4, GL_FLOAT, GL_FALSE, 0,
                          BUFFER_OFFSET(sizeof(vec4) * len));
    glEnableVertexAttribArray(modelVSelected);
    glVertexAttribPointer(modelVSelected, 1, GL_FLOAT, GL_FALSE, 0,
                          BUFFER_OFFSET(sizeof(vec4) * len * 2));
    glEnableVertexAttribArray(modelVindex);
    glVertexAttribPointer(
        modelVindex, 1, GL_FLOAT, GL_FALSE, 0,
        BUFFER_OFFSET((sizeof(vec4) * len * 2) + (sizeof(GLfloat) * len)));


    glDrawArrays(GL_TRIANGLES, 0, MODEL_NUM_VERTICES);
  }
  glFlush();
}

void update(void) {}

//----------------------------------------------------------------------------
// exit when escape key pressed
void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods) {
  switch (key) {
  case GLFW_KEY_ESCAPE:
    exit(EXIT_SUCCESS);
    break;

  case GLFW_KEY_W:
    translateCamera(0.0, 0.0, -0.2);
    break;
    // move around with wasd, go up down with q e, look left right with n m
  case GLFW_KEY_S:
    translateCamera(0.0, 0.0, 0.2);
    break;
  case GLFW_KEY_A:
    translateCamera(-0.2, 0.0, 0.0);
    break;
  case GLFW_KEY_D:
    translateCamera(0.2, 0.0, 0.0);
    break;
  case GLFW_KEY_Q:
    translateCamera(0.0, -0.2, 0.0);
    break;
  case GLFW_KEY_E:
    translateCamera(0.0, 0.2, 0.0);
    break;
  case GLFW_KEY_N:
    rotateCamera(Ry, -2);
    break;
  case GLFW_KEY_M:
    rotateCamera(Ry, 2);
    break;
    // rotate up and down by j k
  case GLFW_KEY_J:
    rotateCamera(Rx, -2);
    break;
  case GLFW_KEY_K:
    rotateCamera(Rx, 2);
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

  if (xpos > width - UI_WIDTH)
    return;

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
      if (xpos > width - UI_WIDTH)
        return;
      
      // draw with unique colors
      display(true);

      unsigned char data[4];
      glReadPixels(xpos, height - ypos, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
      vec4 color = vec4(data[0] / 255.0, data[1] / 255.0, data[2] / 255.0, 1.0);
      int index = UNIQUE_COLOR_TO_INT(color);

      if (index < MODEL_NUM_VERTICES) {
        selected[index*3] = selected[index*3] == 0.0 ? 1.0 : 0.0;
        selected[(index*3)+1] = selected[(index*3)+1] == 0.0 ? 1.0 : 0.0;
        selected[(index*3)+2] = selected[(index*3)+2] == 0.0 ? 1.0 : 0.0;
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
mat4 on_get_leftsize_ortho(GLint width, GLint height) {
  mat4 r = identity();
  r[0][0] = (((GLfloat)width) - UI_WIDTH) / (GLfloat)(width);
  r[0][3] = -UI_WIDTH / width;
  std::cout << r << std::endl;
  return r;
}

void on_window_resize(GLFWwindow *window, GLint width, GLint height) {
  if (width > 800) {
    UI_WIDTH = ((GLfloat)width) / 4.0;
  } else {
    UI_WIDTH = 200;
  }
  GLfloat aspectRatio = ((GLfloat)width - UI_WIDTH) / ((GLfloat)height);
  std::cout << width << " " << height << " " << aspectRatio << std::endl;
  GLfloat FoV;
  if (aspectRatio < 1) {
    // GPT generated dynamic FoV line
    FoV =
        2 * atan(tan(90 * (M_PI / 180) / 2) * (1 / aspectRatio)) * (180 / M_PI);
  } else {
    FoV = 90.0;
  }
  projection = on_get_leftsize_ortho(width, height) *
               Perspective(FoV, aspectRatio, 0.1, 200);
  glViewport(0, 0, width, height);
}

//----------------------------------------------------------------------------
float diffuse_chooser[3];
float specular_chooser[3];

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
  selected =
      (GLfloat *)malloc(main_model.triangles.size() * 3 * sizeof(GLfloat));

  for (int i = 0; i < main_model.triangles.size() * 3; i++) {
    selected[i] = 0.0;
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
}

void invert_model_normals() {
  invert_normals(&main_model);
  generate_normals(normals, &main_model, true);
}

//----------------------------------------------------------------------------

int main(int argc, char **argv) {
  if (!glfwInit())
    exit(EXIT_FAILURE);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  GLFWwindow *window =
      glfwCreateWindow(500 + UI_WIDTH, 500, "Rubix Cube", NULL, NULL);
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

  glfwSetWindowSizeLimits(window, 400 + UI_WIDTH, 400, GLFW_DONT_CARE,
                          GLFW_DONT_CARE);

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
    ImGui::SetNextWindowPos(ImVec2(width - UI_WIDTH, 0));
    ImGui::SetNextWindowSize(ImVec2(UI_WIDTH, height));
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
      ImGui::BeginChild("Model Properties Box", ImVec2(-1, -1), true,
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
