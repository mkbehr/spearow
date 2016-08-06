#include <cstddef>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <cassert>
#include <stdnoreturn.h>

#include <unistd.h>
#include <sys/param.h>

#include "screen.hpp"
#include "mem.hpp"

const char *PROGRAM_NAME = "Spearow";
const char *VERTEX_SHADER_FILE = "vertex.vert";
const char *FRAGMENT_SHADER_FILE = "fragment.frag";

#define INNER_STRINGIZE(x) #x
#define STRINGIZE(x) INNER_STRINGIZE(x)
#define checkGlErrors(cont) \
  (_checkGlErrors(cont, __FILE__ ":" STRINGIZE(__LINE__)))

noreturn void die(void) {
  glfwTerminate();
  exit(-1);
}

const char *glErrorString(GLenum err) {
  switch (err) {
  case GL_INVALID_ENUM:
    return "Invalid enum";
  case GL_INVALID_VALUE:
    return "Invalud value";
  case GL_INVALID_OPERATION:
    return "Invalid operation";
  case GL_OUT_OF_MEMORY:
    return "Out of memory";
  case GL_INVALID_FRAMEBUFFER_OPERATION:
    return "Invalid framebuffer operation";
  default:
    return "Unrecognized error";
  }
}

void _checkGlErrors(int continue_after_err,
                    const char *errloc) {
  GLenum err = GL_NO_ERROR;
  while ((err = glGetError()) != GL_NO_ERROR) {
    const char *errMsg = glErrorString(err);
    std::cerr << errloc
              << ": GL error: " << errMsg
              << " (" << std::hex << err << ")\n";
    if (!continue_after_err) {
      die();
    }
  }
}

Screen::Screen(CPU *c)
  : cpu(c)
{
  // Bit of a hack here: initializing the window will change the
  // working directory for some reason, so store it and change it back
  char cwd[MAXPATHLEN];
  if (!getcwd(cwd, MAXPATHLEN)) {
    std::cerr << "Couldn't get working directory\n";
    die();
  }

  if (initWindow(&window) < 0) {
    std::cerr << "Couldn't create window\n";
    die();
  }

  if (chdir(cwd) != 0) {
    std::cerr << "Couldn't reset working directory\n";
    die();
  }

    // bind a vertex array
  GLuint va;
  glGenVertexArrays(1, &va);
  checkGlErrors(0);
  glBindVertexArray(va);
  checkGlErrors(0);

  initShaders();


  glGenBuffers(1, &bgVbo);
  glGenTextures(1, &texName);

  checkGlErrors(0);

  glClearColor(0.0,0.0,0.0,0.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glfwSwapBuffers(window);
  glfwPollEvents();
  checkGlErrors(0);
}

void Screen::draw() {
  const uint16_t bg_base = (lcd_control & LCDC_BG_CODE)
    ? 0x9c00 : 0x9800;
  const uint16_t tile_base = 0x8000;
  const bool tile_signed = !!(lcd_control & LCDC_BG_CHR);

  // start slow, make it work

  float pixels[144*160];

  for (int y = 0; y < 144; y++) {
    int tileY = y / 8;
    for (int x = 0; x < 160; x++) {
      int tileX = x / 8;
      int block = tileY * 32 + tileX;


      uint8_t tile_code = gb_mem_ptr(*cpu, bg_base + block).read();
      int tile_n = tile_signed ? (int8_t) tile_code : tile_code;

      uint16_t tile_addr = tile_base + tile_n * 16;

      uint8_t low_byte = gb_mem_ptr(*cpu, tile_addr + (y%8)*2).read();
      uint8_t high_byte = gb_mem_ptr(*cpu, tile_addr + (y%8)*2 + 1).read();

      int out = 0;
      if (low_byte & (1<<(7-(x%8)))) {
        out += 1;
      }
      if (high_byte & (1<<(7-(x%8)))) {
        out += 2;
      }
      pixels[x + (y*160)] = out / 3.0;
    }
  }

  glBindBuffer(GL_ARRAY_BUFFER, bgVbo);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texName);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 160, 144,
               0, GL_RED, GL_FLOAT, pixels);
  checkGlErrors(0);

  glClearColor(0.0,0.0,0.0,0.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // TODO what does this do again?
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  checkGlErrors(0);

  glActiveTexture(GL_TEXTURE0);
  checkGlErrors(0);
  glBindTexture(GL_TEXTURE_2D, texName); // do we need this twice?
  checkGlErrors(0);

  vertex vs[6];
  vs[0] = {0.0,0.0};
  vs[1] = {0.0,1.0};
  vs[2] = {1.0,1.0};
  vs[3] = {0.0,0.0};
  vs[4] = {1.0,1.0};
  vs[5] = {1.0,0.0};

  int stride = sizeof(vertex);
  glBufferData(GL_ARRAY_BUFFER, 6 * stride, vs, GL_DYNAMIC_DRAW);
  checkGlErrors(0);
  glVertexAttribPointer(posAttrib, 2,
                        GL_FLOAT, GL_FALSE, stride,
                        (const GLvoid *) offsetof(vertex, x));
  glEnableVertexAttribArray(posAttrib);
  checkGlErrors(0);
  glUniform1i(texUniform, 0); // 0 corresponds to GL_TEXTURE0
  checkGlErrors(0);

  glDrawArrays(GL_TRIANGLES, 0, 6);
  checkGlErrors(0);

  glfwSwapBuffers(window);
  glfwPollEvents();
  if (glfwWindowShouldClose(window)) {
    die();
  }
}

int initWindow(GLFWwindow **window_p) {
  /* Initialize the library */
  if (!glfwInit()) {
    std::cerr << "glfwInit failed\n";
    return -1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  /* Create a windowed mode window and its OpenGL context */
  GLFWwindow *window;
  window = glfwCreateWindow(SCREEN_WIDTH*2, SCREEN_HEIGHT*2,
                            PROGRAM_NAME, NULL, NULL);
  if (!window)
    {
      std::cerr << "glfwCreateWindow failed\n";
      glfwTerminate();
      return -1;
    }

  if (glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MAJOR) < 3) {
    std::cerr << "initWindow: Error: GL major version too low\n";
    die();
  }

  /* Make the window's context current */
  glfwMakeContextCurrent(window);

  *window_p = window;
  return 0;
}

GLint safeGetAttribLocation(GLuint program, const GLchar *name) {
  GLint loc = glGetAttribLocation(program, name);
  checkGlErrors(0);
  if (loc < 0) {
    std::cerr << "Couldn't get program attribute " << name << "\n";
    die();
  }
  return loc;
}

GLint safeGetUniformLocation(GLuint program, const GLchar *name) {
  GLint loc = glGetUniformLocation(program, name);
  checkGlErrors(0);
  if (loc < 0) {
    std::cerr << "Couldn't get program uniform " << name << "\n";
    die();
  }
  return loc;
}

void Screen::initShaders(void) {
  // get shaders
  std::ifstream vertFile(VERTEX_SHADER_FILE);
  if (!vertFile) {
    std::cerr << "Couldn't open vertex shader file\n";
    die();
  }
  std::stringstream vertBuffer;
  vertBuffer << vertFile.rdbuf();
  std::string vertStr = vertBuffer.str();
  const char *vertSrc = vertStr.c_str();
  // TODO error-check and make sure the file isn't too big
  int vertSrcLen = (int) vertStr.length();
  std::ifstream fragFile(FRAGMENT_SHADER_FILE);
  if (!fragFile) {
    std::cerr << "Couldn't open fragment shader file\n";
    die();
  }
  std::stringstream fragBuffer;
  fragBuffer << fragFile.rdbuf();
  std::string fragStr = fragBuffer.str();
  const char *fragSrc = fragStr.c_str();
  int fragSrcLen = (int) fragStr.length();

  // compile shaders
  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertSrc, &vertSrcLen);
  glCompileShader(vertexShader);
  // check compilation
  GLint status;
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
  if (!status) {
    std::cerr << "Error compiling vertex shader:\n";
    GLint logLen = 0;
    glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &logLen);
    if (logLen > 1) {
      GLchar *log = (GLchar*) malloc(logLen);
      GLint readLogLen = 0;
      glGetShaderInfoLog(vertexShader, logLen, &readLogLen, log);
      // FIXME should probably only print logLen chars here just to be safe
      std::cout << log;
    } else {
      std::cerr << "Couldn't get log message\n";
    }
    die();
  }

  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragSrc, &fragSrcLen);
  glCompileShader(fragmentShader);
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
  if (!status) {
    std::cerr << "Error compiling fragment shader:\n";
    GLint logLen = 0;
    glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &logLen);
    if (logLen > 1) {
      GLchar *log = (GLchar*) malloc(logLen);
      GLint readLogLen = 0;
      glGetShaderInfoLog(fragmentShader, logLen, &readLogLen, log);
      // FIXME should probably only print logLen chars here just to be safe
      std::cout << log;
    } else {
      std::cerr << "Couldn't get log message\n";
    }
    die();
  }

  // link shader program
  shader = glCreateProgram();
  glAttachShader(shader, vertexShader);
  glAttachShader(shader, fragmentShader);
  glLinkProgram(shader);

  glGetProgramiv(shader, GL_LINK_STATUS, &status);
  if (!status) {
    std::cerr << "Error linking shader:\n";
    GLint logLen = 0;
    glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &logLen);
    if (logLen > 1) {
      GLchar *log = (GLchar*) malloc(logLen);
      GLint readLogLen = 0;
      glGetProgramInfoLog(shader, logLen, &readLogLen, log);
      // FIXME should probably only print logLen chars here just to be safe
      std::cout << log;
    } else {
      std::cerr << "Couldn't get log message\n";
    }
    die();
  }

  glUseProgram(shader);
  checkGlErrors(0);

  // shader attribute/uniform pointers
  posAttrib = safeGetAttribLocation(shader, "pos");

  texUniform = safeGetUniformLocation(shader, "tex");
}