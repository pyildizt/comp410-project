#ifndef _SHADED_OBJECT_HPP
#define _SHADED_OBJECT_HPP

#include "data_types.hpp"
#include "load_model.hpp"
#include "model_projections.hpp"
#if defined(__linux__) || defined(_WIN32)
    #include <GL/gl.h>
#elif defined(__APPLE__)
#endif
#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

#define OPGL(x)                                                                \
  {                                                                            \
    x;                                                                         \
    auto err = glGetError();                                                   \
    if (err != 0) {                                                            \
      std::cout << "GL ERROR: " << __LINE__ << " " << err << " "               \
                /* FIXME: << glewGetErrorString(err) */<< std::endl;                       \
      exit(1);                                                                 \
    }                                                                          \
  }

namespace sobj {
/**
 * @brief The shaded_object struct holds <mostly> everything you would need to
 * render a model.
 *
 * In order to use it, you need to give it two shader programs: Program (for
 * normal display) and PickerProgram (for picking). For the picker program, you
 * need to set the unique_color variable to a unique color.
 */
struct shaded_object {
public:
  model inner_model;

  mat4 transform;

  vec4 unique_color;

  GLuint VAO;
  GLuint VBO;
  GLuint Program;
  GLuint PickerProgram = 0;
  GLuint TexturePointer; // when loading the texture

  GLuint vPositionPointer; // vertex-attirb vec4
  GLuint NormalPointer;    // vertex-attrib vec4
  GLuint UVpointer;        // vertex-attrib vec4

  GLuint PickerFakeColorPointer; // uniform vec4
  GLuint PickerVPositionPointer; // vertex-attrib vec4
  GLuint PickerProjectionPointer; // uniform mat4
  GLuint PickerTransformPointer;  // uniform mat4

  GLuint ProjectionPointer;    // uniform mat4
  GLuint TransformPointer;     // uniform mat4
  GLuint DiffuseMultPointer;   // uniform vec4
  GLuint SpecularMultPointer;  // uniform vec4
  GLuint AmbientMultPointer;   // uniform vec4
  GLuint ShininessPointer;     // uniform float
  GLuint ShaderTextureSampler; // uniform int

  GLuint LightPositionPointer; // uniform vec4
  GLuint HasTexturePointer;    // uniform int

  /**
   * @brief Loads the model from the given json file. Note that the model is
   * still not ready to use until you run the other initialization functions.
   *
   * @param filename
   */
  void load_model_from_json(const char *filename) {
    std::ifstream file2(filename);
    std::stringstream buffer;
    buffer << file2.rdbuf();
    std::string json2 = buffer.str();
    file2.close();

    serializable_model smodel;
    smodel.zax_from_json(json2.c_str());

    std::cout << "Loaded Model " << smodel << std::endl;

    inner_model = to_model(smodel);
  }

  /**
   * @brief don't worry about this function. It's just a helper function for the
   * class.
   *
   * @param vertices
   * @param normals
   * @param uvs
   * @param len_verts
   */
  void temp_pointers(vec4 *&vertices, vec4 *&normals, vec4 *&uvs,
                     int &len_verts) {
    OPGL();
    renormalizeUVs(&inner_model);
    to_point_array(&vertices, &len_verts, &inner_model.triangles);

    vec2 *tmp;
    to_point_array_uv(&tmp, &inner_model.triangles);
    uvs = (vec4 *)malloc(sizeof(vec4) * len_verts);
    for (int i = 0; i < len_verts; i++) {
      uvs[i] = vec4(tmp[i].x, tmp[i].y, 2.0, 1.0);
    }
    free(tmp);

    normals = (vec4 *)malloc(inner_model.triangles.size() * 3 * sizeof(vec4));
    generate_normals(normals, &inner_model, true);
    OPGL();
  }

  /**
   * @brief loads the vertices, normals and uv informations into the VBO. You
   * need to run this if you update normals or uvs.
   *
   * @param vcount
   */
  void load_buffer_data(int vcount) {
    OPGL();
    vec4 *vertices;
    vec4 *normals;
    vec4 *uvs;
    int len_verts;
    temp_pointers(vertices, normals, uvs, len_verts);

    glBufferData(GL_ARRAY_BUFFER, vcount * 3 * sizeof(vec4), NULL,
                 GL_STATIC_DRAW);
    int buffer_offset = 0;
    glBufferSubData(GL_ARRAY_BUFFER, buffer_offset, vcount * sizeof(vec4),
                    vertices);
    buffer_offset += vcount * sizeof(vec4);

    glBufferSubData(GL_ARRAY_BUFFER, buffer_offset, vcount * sizeof(vec4),
                    normals);
    buffer_offset += vcount * sizeof(vec4);

    glBufferSubData(GL_ARRAY_BUFFER, buffer_offset, vcount * sizeof(vec4), uvs);

    free(vertices);
    free(normals);
    free(uvs);
    OPGL();
  }

  /**
   * @brief Sets up uniform/attrib locations for the program. The program must
   * be set before calling this function.
   *
   */
  void setup_program_pointers() {
    OPGL();
    glUseProgram(Program);
    // load pointers
    // array pointers
    vPositionPointer = glGetAttribLocation(Program, "vPosition");
    NormalPointer = glGetAttribLocation(Program, "vNormal");
    UVpointer = glGetAttribLocation(Program, "uvs");
    // uniform pointers
    ProjectionPointer = glGetUniformLocation(Program, "Projection");
    TransformPointer = glGetUniformLocation(Program, "Transform");
    DiffuseMultPointer = glGetUniformLocation(Program, "DiffuseProduct");
    SpecularMultPointer = glGetUniformLocation(Program, "SpecularProduct");
    AmbientMultPointer = glGetUniformLocation(Program, "AmbientProduct");
    ShininessPointer = glGetUniformLocation(Program, "Shininess");
    LightPositionPointer = glGetUniformLocation(Program, "LightPosition");
    HasTexturePointer = glGetUniformLocation(Program, "HasTexture");
    ShaderTextureSampler = glGetUniformLocation(Program, "texture1");
    OPGL();
  }

  /**
   * @brief Set up the pointers for the picker program. The picker program must
   * be set before calling this function. The picker program is the "shader"
   * that allows you to pick objects with unique colors.
   *
   */
  void setup_picker_pointers() {
    OPGL();
    if (PickerProgram != 0) {
      glUseProgram(PickerProgram);
      //TODO change this to whatever the picker program uses
      PickerFakeColorPointer = glGetUniformLocation(PickerProgram, "pColor");
      PickerProjectionPointer = glGetUniformLocation(PickerProgram, "Projection");
      PickerTransformPointer = glGetUniformLocation(PickerProgram, "Transform");
      PickerVPositionPointer = glGetAttribLocation(PickerProgram, "vPosition");
    }
    OPGL();
  }

  /**
   * @brief Assumes the program is already set, and sets up all pointers, puts
   * all the data in the VBO, and sets up the VAO.
   *
   */
  void initModel() {
    OPGL();
    assert(!inner_model.is_empty());

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // put vertex-arrays in the buffer
    int vcount = inner_model.triangles.size() * 3;
    load_buffer_data(vcount);

    setup_program_pointers();
    setup_picker_pointers();
    OPGL();
  }

  /**
   * @brief Displays the model with the given transform, projection, and light
   * position.
   *
   * @param transform
   * @param projection
   * @param lightPosition currently only supports one light
   */
  void display_real(mat4 transform, mat4 projection, vec4 lightPosition) {
    OPGL();
    OPGL(glUseProgram(Program));
    OPGL(glBindVertexArray(VAO));
    OPGL(glBindBuffer(GL_ARRAY_BUFFER, VBO));

    OPGL(glUniformMatrix4fv(ProjectionPointer, 1, GL_TRUE, projection));
    OPGL(glUniformMatrix4fv(TransformPointer, 1, GL_TRUE, transform));

    OPGL(glUniform4fv(LightPositionPointer, 1, lightPosition));

    OPGL(glUniform4fv(DiffuseMultPointer, 1, inner_model.material.diffuse));
    OPGL(glUniform4fv(SpecularMultPointer, 1, inner_model.material.specular));
    OPGL(glUniform4fv(AmbientMultPointer, 1, inner_model.material.ambient));
    OPGL(glUniform1f(ShininessPointer, inner_model.material.shininess));

    bool has_texture = (TexturePointer < (unsigned)-1);
    OPGL(glUniform1i(HasTexturePointer, (int)has_texture));

    if (has_texture) {
      OPGL(glActiveTexture(GL_TEXTURE0));
      OPGL(glBindTexture(GL_TEXTURE_2D, TexturePointer));
      OPGL(glUniform1i(ShaderTextureSampler, 0));
      glGenerateMipmap(GL_TEXTURE_2D);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    OPGL(glEnableVertexAttribArray(vPositionPointer));
    OPGL(glVertexAttribPointer(vPositionPointer, 4, GL_FLOAT, GL_FALSE, 0,
                               BUFFER_OFFSET(0)));

    if (NormalPointer < (unsigned)-1) {
      OPGL(glEnableVertexAttribArray(NormalPointer));
      OPGL(glVertexAttribPointer(
          NormalPointer, 4, GL_FLOAT, GL_FALSE, 0,
          BUFFER_OFFSET(inner_model.triangles.size() * 3 * sizeof(vec4))));
    }
    if (UVpointer < (unsigned)-1) {
      OPGL(glEnableVertexAttribArray(UVpointer));
      OPGL(glVertexAttribPointer(
          UVpointer, 4, GL_FLOAT, GL_FALSE, 0,
          BUFFER_OFFSET(inner_model.triangles.size() * 3 * sizeof(vec4) * 2)));
    }
    OPGL(glDrawArrays(GL_TRIANGLES, 0, inner_model.triangles.size() * 3));
  }

  /**
   * @brief Draws the model with the given transform and projection, but with a
   * unique color, for picking.
   *
   * @param transform
   * @param projection
   */
  void display_picker(mat4 transform, mat4 projection) {
    OPGL();
    if (PickerProgram == 0) {
      return;
    }
    OPGL(glUseProgram(PickerProgram));
    OPGL(glBindVertexArray(VAO));
    OPGL(glBindBuffer(GL_ARRAY_BUFFER, VBO));

    OPGL(glUniformMatrix4fv(PickerProjectionPointer, 1, GL_TRUE, projection));
    OPGL(glUniformMatrix4fv(PickerTransformPointer, 1, GL_TRUE, transform));
    OPGL(glUniform4fv(PickerFakeColorPointer, 1, unique_color));

    OPGL(glEnableVertexAttribArray(PickerVPositionPointer));
    OPGL(glVertexAttribPointer(PickerVPositionPointer, 4, GL_FLOAT, GL_FALSE, 0,
                               BUFFER_OFFSET(0)));

    OPGL(glDrawArrays(GL_TRIANGLES, 0, inner_model.triangles.size() * 3));
  }
};
}; // namespace sobj

#endif
