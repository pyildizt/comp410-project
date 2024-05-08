#ifndef _SIMPLE_WINDOWING_HPP
#define _SIMPLE_WINDOWING_HPP

#include "load_model.hpp"

typedef enum { ABSOLUTE, PERCENT, RATIO } scale_type;

typedef struct elastic_value {
  float value;
  scale_type type;

public:
  float get_absolute(float parent_absolute, const struct elastic_value *other) {
    switch (type) {
    case ABSOLUTE:
      return value;
    case PERCENT:
      return (parent_absolute)*value / 100;
    case RATIO:
      if (other->type == RATIO) {
        std::cerr << "Both width and height are RATIO in window!" << std::endl;
        exit(1);
      }
    }
    return 0.0;
  }
} e_val;

struct subwindow {
  e_val width;
  e_val height;
  e_val xoffset;
  e_val yoffset;
  int z_index;
  struct subwindow *parent;

public:
  float get_absolute_width() {
    if (parent == nullptr) {
      return width.value;
    }
    return width.get_absolute(parent->get_absolute_width(), &height);
  }
  float get_absolute_height() {
    if (parent == nullptr) {
      return height.value;
    }
    return height.get_absolute(parent->get_absolute_height(), &width);
  }
  float get_x_offset() {
    if (parent == nullptr) {
      return xoffset.value;
    }
    return xoffset.get_absolute(parent->get_absolute_width(), &yoffset);
  }
  float get_y_offset() {
    if (parent == nullptr) {
      return yoffset.value;
    }
    return yoffset.get_absolute(parent->get_absolute_height(), &xoffset);
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

  mat4 get_transform_matrix() {
    mat4 parent_mat;
    float parent_width;
    float parent_height;
    if (parent == nullptr) {
      parent_mat = identity();
      parent_width = 1;
      parent_height = 1;
    } else {
      parent_mat = parent->get_transform_matrix();
      parent_width = parent->get_absolute_width();
      parent_height = parent->get_absolute_height();
    }
    mat4 transform = Translate(vec3(get_x_offset(), get_y_offset(), 1)) *
                     Scale(vec3(get_absolute_width() / parent_width,
                                get_absolute_height() / parent_height, 1));
    return transform * parent_mat;
  }
};

#endif
