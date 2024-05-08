#ifndef _SIMPLE_WINDOWING_HPP
#define _SIMPLE_WINDOWING_HPP

#include "load_model.hpp"

typedef enum { ABSOLUTE, PERCENT, RATIO } scale_type;

class elastic_value {

public:
  float value;
  scale_type type;

  float get_absolute(float parent_absolute, const elastic_value *other) {
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

  elastic_value(float value, scale_type type) : value(value), type(type) {}
};

class subwindow {
  elastic_value width;
  elastic_value height;
  elastic_value xoffset;
  elastic_value yoffset;
  subwindow *parent;

public:
  subwindow(elastic_value width, elastic_value height, elastic_value xoffset,
            elastic_value yoffset, subwindow *parent)
      : width(width), height(height), xoffset(xoffset), yoffset(yoffset),
        parent(parent) {}

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

  mat4 get_scale_matrix() {
    mat4 parent_mat;
    float parent_width;
    float parent_height;
    if (parent == nullptr) {
      parent_mat = identity();
      parent_width = 1;
      parent_height = 1;
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
    if (parent == nullptr) {
      parent_mat = identity();
    } else {
      parent_mat = parent->get_translate_matrix();
    }
    mat4 transform = Translate(vec3(get_x_offset(), get_y_offset(), 1));
    return transform * parent_mat;
  }

  mat4 get_transform_matrix() {
    mat4 translate = get_translate_matrix();
    mat4 scale = get_scale_matrix();
    return scale * translate;
  }
  /*
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
  }*/
};

#endif
