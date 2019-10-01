/*
 * Copyright (c) 2019 Nobuyuki Umetani
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <stdio.h>
#include <cstdlib>
#include <cassert>
#include <math.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#ifdef USE_GLEW
  #include <GL/glew.h>
#else
  #include <glad/glad.h>
  #ifdef EMSCRIPTEN
    #include <emscripten/emscripten.h>
    #define GLFW_INCLUDE_ES3
  #endif
#endif

#include "delfem2/gl24_funcs.h"


// ---------------------------------------------------------------------------


int GL24_CompileShader(const char *vert, const char* frag)
{ // build and compile our shader program
  // ------------------------------------
  // vertex shader
  int vertexShader = glCreateShader(GL_VERTEX_SHADER); // gl24
  glShaderSource(vertexShader, 1, &vert, NULL); // gl24
  glCompileShader(vertexShader); // gl24
  // check for shader compile errors
  int success;
  char infoLog[512];
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success); // gl24
  if (!success)
  {
    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog); // gl24
    std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
  }
  // fragment shader
  int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER); //gl24
  glShaderSource(fragmentShader, 1, &frag, NULL); // gl24
  glCompileShader(fragmentShader); // gl24
  // check for shader compile errors
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success); // gl24
  if (!success)
  {
    glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog); // gl24
    std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
  }
  // link shaders
  int shaderProgram = glCreateProgram(); // gl24
  glAttachShader(shaderProgram, vertexShader); // gl24
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);
  // check for linking errors
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
  }
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);
  return shaderProgram;
}



int compileShader
(const std::string& str_glsl_vert,
 int shaderType)
{
  int id_shader = glCreateShader(shaderType);
  const char *vfile = str_glsl_vert.c_str();
  glShaderSource(id_shader, 1, &vfile, NULL);
  glCompileShader(id_shader); // compile the code
  
  {
    GLint res;
    glGetShaderiv(id_shader, GL_COMPILE_STATUS, &res);
    if (res==GL_FALSE){
      if (shaderType==GL_VERTEX_SHADER){
        std::cout<<"compile vertex shader failed"<<std::endl;
      }
      else if(shaderType==GL_FRAGMENT_SHADER){
        std::cout<<"compile fragment shader failed"<<std::endl;
      }
    }
  }
  return id_shader;
}

// compile vertex and fragment shader
// return shader program
int setUpGLSL
(const std::string& str_glsl_vert,
 const std::string& str_glsl_frag)
{
  int vShaderId = compileShader(str_glsl_vert, GL_VERTEX_SHADER);
  int fShaderId = compileShader(str_glsl_frag, GL_FRAGMENT_SHADER);
  
  
  int id_program = glCreateProgram();
  glAttachShader(id_program,vShaderId);
  glAttachShader(id_program,fShaderId);
  
  GLint linked;
  glLinkProgram(id_program);
  glGetProgramiv(id_program, GL_LINK_STATUS, &linked);
  if(linked == GL_FALSE)
  {
    std::cerr << "Link Err.\n";
    GLint maxLength = 0;
    glGetProgramiv(id_program, GL_INFO_LOG_LENGTH, &maxLength);
    // The maxLength includes the NULL character
    std::vector<GLchar> infoLog(maxLength);
    glGetProgramInfoLog(id_program, maxLength, &maxLength, &infoLog[0]);
    for(unsigned int i=0;i<infoLog.size();++i){
      std::cout << infoLog[i];
    }
    std::cout << std::endl;
    glDeleteProgram(id_program); // The program is useless now. So delete it.
    return 0;
  }
  return id_program;
}
