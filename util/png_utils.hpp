#ifndef _PNG_UTILS_HPP

#define _PNG_UTILS_HPP

#include <png.h>
#include "load_model.hpp"

// load png as OpenGL texture
GLuint loadPNG(const char *file_name, int* width, int* height);

#endif