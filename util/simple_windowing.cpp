#include "simple_windowing.hpp"


GLuint swind::WindowSeperatingLineShader = 0;
GLuint swind::WindowSeperatingLineVAO = 0;
GLuint swind::WindowSeperatingLineVBO = 0;
GLuint swind::WindowSeperatingLineTransform = 0;
GLuint swind::WindowSeperatingLineVPosition = 0;

// GPT generated code
GLuint CreateShaderProgram(const char *vertex_shader,
                           const char *fragment_shader) {

  // Create vertex shader
  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertex_shader, nullptr);
  glCompileShader(vertexShader);

  // Check for vertex shader compilation errors
  GLint success;
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
    std::cerr << "Vertex shader compilation failed:\n" << infoLog << std::endl;
    return 0;
  }

  // Create fragment shader
  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragment_shader, nullptr);
  glCompileShader(fragmentShader);

  // Check for fragment shader compilation errors
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
    std::cerr << "Fragment shader compilation failed:\n"
              << infoLog << std::endl;
    return 0;
  }

  // Create shader program
  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);

  // Check for shader program linking errors
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
    std::cerr << "Shader program linking failed:\n" << infoLog << std::endl;
    return 0;
  }

  // Delete the shaders as they are now linked to the program and no longer
  // needed
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  return shaderProgram;
}