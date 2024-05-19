#ifndef _SIMPLE_WINDOWING_HPP
#define _SIMPLE_WINDOWING_HPP

#include "load_model.hpp"
#include "mat.h"
#include <algorithm>
#include <float.h>

namespace swind {
typedef enum { ABSOLUTE, PERCENT, RATIO } scale_type;
typedef enum { MAX, MIN } elastic_value_default;
typedef enum { HORIZONTAL, VERTICAL } tiling;

extern GLuint WindowSeperatingLineShader;
extern GLuint WindowSeperatingLineVAO;
extern GLuint WindowSeperatingLineVBO;
extern GLuint WindowSeperatingLineTransform;
extern GLuint WindowSeperatingLineVPosition;

const char vertex_shader[] = "#version 330 core\n"
                             "layout (location = 0) in vec4 aPos;\n"
                             "uniform mat4 transform;\n"
                             "void main()\n"
                             "{\n"
                             "   gl_Position = transform * aPos;\n"
                             "}\0";

const char fragment_shader[] = "#version 330 core\n"
                               "out vec4 FragColor;\n"
                               "void main()\n"
                               "{\n"
                               "   FragColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
                               "}\n\0";
const vec4 window_seperating_line[] = {
    vec4(-1.0, -1.0, 0.0, 1.0), vec4(1.0, -1.0, 0.0, 1.0),
    vec4(1.0, 1.0, 0.0, 1.0),   vec4(-1.0, 1.0, 0.0, 1.0),
    vec4(-1.0, -1.0, 0.0, 1.0), vec4(1.0, 1.0, 0.0, 1.0),
};
} // namespace swind

GLuint CreateShaderProgram(const char *vertex_shader,
                           const char *fragment_shader);

#define SETUP_SWIND_LINE_SHADER()                                              \
  {                                                                            \
    swind::WindowSeperatingLineShader =                                        \
        CreateShaderProgram(swind::vertex_shader, swind::fragment_shader);     \
    glGenVertexArrays(1, &swind::WindowSeperatingLineVAO);                     \
    glGenBuffers(1, &swind::WindowSeperatingLineVBO);                          \
    glBindVertexArray(swind::WindowSeperatingLineVAO);                         \
    glBindBuffer(GL_ARRAY_BUFFER, swind::WindowSeperatingLineVBO);             \
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6, nullptr, GL_STATIC_DRAW); \
    swind::WindowSeperatingLineVPosition =                                     \
        glGetAttribLocation(swind::WindowSeperatingLineShader, "aPos");        \
    glEnableVertexAttribArray(swind::WindowSeperatingLineVPosition);           \
    glVertexAttribPointer(swind::WindowSeperatingLineVPosition, 4, GL_FLOAT,   \
                          GL_FALSE, 0, BUFFER_OFFSET(0));                      \
    swind::WindowSeperatingLineTransform =                                     \
        glGetUniformLocation(swind::WindowSeperatingLineShader, "transform");  \
  }

struct elastic_value {

public:
  float value;
  swind::scale_type type;

  float get_absolute(float parent_absolute, float other_size) {
    switch (type) {
    case swind::ABSOLUTE:
      return value;
    case swind::PERCENT:
      return parent_absolute * value / 100;
    case swind::RATIO:
      if (other_size < 0.0) {
        std::cerr << "Error: RATIO value used without other size" << std::endl;
        exit(1);
      }
      return value * other_size / 100;
    }
    return 0.0;
  }

  elastic_value(float value, swind::scale_type type)
      : value(value), type(type) {}

  elastic_value(swind::elastic_value_default type) {
    switch (type) {
    case swind::MIN:
      this->value = 100;
      this->type = swind::PERCENT;
    case swind::MAX:
      this->value = 0;
      this->type = swind::PERCENT;
    }
  }

  elastic_value() : value(0), type(swind::ABSOLUTE) {}
};

struct elastic_scale {
  elastic_value min;
  elastic_value regular;
  elastic_value max;

public:
  elastic_value *get_active(float parent_absolute, elastic_scale *other) {
    auto other_absolute = other->get_absolute(parent_absolute, nullptr);
    auto min_val = min.get_absolute(parent_absolute, other_absolute);
    auto max_val = max.get_absolute(parent_absolute, other_absolute);
    auto regular_val = regular.get_absolute(parent_absolute, other_absolute);

    if (min_val > regular_val) {
      return &min;
    } else if (max_val < regular_val) {
      return &max;
    } else {
      return &regular;
    }
  }

  float get_absolute(float parent_absolute, elastic_scale *other) {
    float other_size;
    if (other == nullptr) {
      other_size = -1;
    } else {
      other_size = other->get_absolute(parent_absolute, nullptr);
    }
    auto min_val = min.get_absolute(parent_absolute, other_size);
    auto max_val = max.get_absolute(parent_absolute, other_size);
    auto regular_val = regular.get_absolute(parent_absolute, other_size);

    return std::max(min_val, std::min(max_val, regular_val));
  }
  float get() {
    return std::max(min.value, std::min(max.value, regular.value));
  }

  elastic_scale(elastic_value min, elastic_value max, elastic_value regular)
      : min(min), regular(regular), max(max) {}

  elastic_scale() {}
};

struct subwindow {
  elastic_scale width;
  elastic_scale height;
  elastic_scale xoffset;
  elastic_scale yoffset;
  subwindow *parent;
  subwindow *sibling;
  swind::tiling tiling = swind::HORIZONTAL;

public:
  subwindow(elastic_value width, elastic_value height, elastic_value xoffset,
            elastic_value yoffset, subwindow *parent, subwindow *sibling)
      : parent(parent), sibling(sibling) {
    this->width = elastic_scale(elastic_value(swind::MIN),
                                elastic_value(swind::MAX), width);
    this->height = elastic_scale(elastic_value(swind::MIN),
                                 elastic_value(swind::MAX), height);
    this->xoffset = elastic_scale(elastic_value(swind::MIN),
                                  elastic_value(swind::MAX), xoffset);
    this->yoffset = elastic_scale(elastic_value(swind::MIN),
                                  elastic_value(swind::MAX), yoffset);
  }

  subwindow(elastic_scale width, elastic_scale height, elastic_scale xoffset,
            elastic_scale yoffset, subwindow *parent, subwindow *sibling)
      : width(width), height(height), xoffset(xoffset), yoffset(yoffset),
        parent(parent), sibling(sibling) {}

  subwindow(float width, float height) {
    this->width = elastic_scale(elastic_value(0, swind::ABSOLUTE),
                                elastic_value(FLT_MAX, swind::ABSOLUTE),
                                elastic_value(width, swind::ABSOLUTE));
    this->height = elastic_scale(elastic_value(0, swind::ABSOLUTE),
                                 elastic_value(FLT_MAX, swind::ABSOLUTE),
                                 elastic_value(height, swind::ABSOLUTE));
    this->xoffset = elastic_scale(elastic_value(0, swind::ABSOLUTE),
                                  elastic_value(0, swind::ABSOLUTE),
                                  elastic_value(0, swind::ABSOLUTE));
    this->yoffset = elastic_scale(elastic_value(0, swind::ABSOLUTE),
                                  elastic_value(0, swind::ABSOLUTE),
                                  elastic_value(0, swind::ABSOLUTE));
    this->parent = nullptr;
    this->sibling = nullptr;
  }

  void resize_absolute(float width, float height) {
    this->width.regular.value = width;
    this->height.regular.value = height;
  }

  float get_absolute_width() {
    if (parent == nullptr) {
      return width.get();
    }
    return width.get_absolute(parent->get_absolute_width(), &height);
  }
  float get_absolute_height() {
    if (parent == nullptr) {
      return height.get();
    }
    return height.get_absolute(parent->get_absolute_height(), &width);
  }
  float get_x_offset() {
    float local_x;
    if (parent == nullptr) {
      local_x = xoffset.get();
    } else {
      local_x = xoffset.get_absolute(parent->get_absolute_width(), &yoffset);
    }
    float sibling_x = 0;
    if (sibling != nullptr && parent != nullptr &&
        parent->tiling == swind::HORIZONTAL)
      sibling_x = sibling->get_x_offset();
    float buffer = 0;
    if (sibling != nullptr && parent != nullptr &&
        parent->tiling == swind::HORIZONTAL)
      buffer = sibling->get_absolute_width();
    return local_x + sibling_x + buffer;
  }
  float get_y_offset() {
    float local_y;
    if (parent == nullptr) {
      local_y = yoffset.get();
    } else {
      local_y = yoffset.get_absolute(parent->get_absolute_width(), &xoffset);
    }
    float sibling_y = 0;
    if (sibling != nullptr && parent != nullptr &&
        parent->tiling == swind::VERTICAL)
      sibling_y = sibling->get_y_offset();
    float buffer = 0;
    if (sibling != nullptr && parent != nullptr &&
        parent->tiling == swind::VERTICAL)
      buffer = sibling->get_absolute_height();
    return local_y + sibling_y + buffer;
  }

  bool is_point_in(float x, float y) {
    auto x_offset = get_x_offset();
    if (parent != nullptr)
      x_offset += parent->get_x_offset();

    auto y_offset = get_y_offset();
    if (parent != nullptr)
      y_offset += parent->get_y_offset();

    auto awidth = get_absolute_width();
    auto aheight = get_absolute_height();

    return x > x_offset && x < (awidth + x_offset) && y > y_offset &&
           y < (aheight + y_offset);
  }

  mat4 get_scale_matrix() {
    mat4 parent_mat;
    float parent_width;
    float parent_height;
    if (parent == nullptr) {
      parent_mat = identity();
      parent_width = get_absolute_width();
      parent_height = get_absolute_height();
      return identity();
    } else {
      parent_mat = parent->get_scale_matrix();
      parent_width = parent->get_absolute_width();
      parent_height = parent->get_absolute_height();
    }
    mat4 transform = Scale(vec3(get_absolute_width() / parent_width,
                                get_absolute_height() / parent_height, 1));
    return transform * parent_mat;
  }

  mat4 get_translate_matrix() {
    mat4 parent_mat;
    float parent_width;
    float parent_height;
    if (parent == nullptr) {
      parent_mat = identity();
      parent_width = get_absolute_width();
      parent_height = get_absolute_height();
    } else {
      parent_mat = parent->get_translate_matrix();
      parent_width = parent->get_absolute_width();
      parent_height = parent->get_absolute_height();
    }
    mat4 transform = Translate(
        vec3(get_x_offset() * 2 / parent_width, get_y_offset() * 2 / parent_height, 0));
    return transform * parent_mat;
  }

  mat4 get_transform_matrix() {
    mat4 translate = get_translate_matrix();
    mat4 scale = get_scale_matrix();
    mat4 ofset = identity();
    // move scale offset as scale centers
    if (parent != nullptr) {
      auto parent_width = parent->get_absolute_width();
      auto parent_height = parent->get_absolute_height();
      ofset = Translate(vec3(
          (-(-get_absolute_width() + parent_width) / (parent_width)),
          (-(-get_absolute_height() + parent_height) / (parent_height)),
          0));

    }
    return translate * ofset * scale;
  }

private:
  mat4 get_horizontal_edge_matrix() {

    mat4 initial = Scale(1, 1.0, 1);

    initial *= Scale(vec3(get_scale_matrix()[0][0], 1, 1));
    return initial;
  }
  mat4 get_vertical_edge_matrix() {

    mat4 initial = Scale(0.012, 1, 1);

    // initial *=
    //     get_scale_matrix(); // Scale(vec3(1, get_scale_matrix()[1][1], 1));
    return initial;
  }

public:
  void draw_edges() {
    if (parent == nullptr) {
      std::cout << "noaprent" << std::endl;
      return;
    }
    // draw two rectangle edges, depending on tiling
    float parent_width = parent->get_absolute_width();
    float parent_height = parent->get_absolute_height();
    mat4 transform[4];
    if (tiling == swind::VERTICAL) {
      transform[0] = get_horizontal_edge_matrix();
      transform[1] = get_horizontal_edge_matrix();
      // translate for up and down
      transform[1] *=
          Translate(vec3(0, get_absolute_height() * 2 / parent_height, 0));

    } else {
      transform[0] = get_vertical_edge_matrix();
      transform[1] = get_vertical_edge_matrix();
      // translate for left and right

      transform[2] =
          Translate(-vec3(get_absolute_width() / parent_width, 0, 0));
      transform[3] =
          Translate(vec3(get_absolute_width() / parent_width, 0, 0));
    }
    mat4 my_translate = get_transform_matrix();
    glUseProgram(swind::WindowSeperatingLineShader);
    glUniformMatrix4fv(swind::WindowSeperatingLineTransform, 1, GL_TRUE,
                       transform[2] * my_translate * transform[0]);
    glBindVertexArray(swind::WindowSeperatingLineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, swind::WindowSeperatingLineVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(swind::window_seperating_line),
                 swind::window_seperating_line, GL_STATIC_DRAW);
    glEnableVertexAttribArray(swind::WindowSeperatingLineVPosition);
    glVertexAttribPointer(swind::WindowSeperatingLineVPosition, 4, GL_FLOAT,
                          GL_FALSE, 0, BUFFER_OFFSET(0));
    // glBufferSubData(GL_ARRAY_BUFFER, 0,
    // sizeof(swind::window_seperating_line),
    //                 swind::window_seperating_line);
    // auto t1 = my_translate * transform[1];
    // std::cout << my_translate * transform[0] << t1 * vec4(1, 0, 0, 1)
    //           << t1 * vec4(0, 1, 0, 1) << t1 * vec4(1, 1, 0, 1)
    //           << t1 * vec4(0, 0, 0, 1) << std::endl
    //           << "----" << std::endl;
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glUniformMatrix4fv(swind::WindowSeperatingLineTransform, 1, GL_TRUE,
                       transform[3] * my_translate * transform[1]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
  }
};
// namespace swind

#endif
