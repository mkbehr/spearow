#ifndef SCREEN_H

#define SCREEN_H

#define GLFW_INCLUDE_GLCOREARB
// defining GLFW_INCLUDE_GLEXT may also be useful in the future
#include <GLFW/glfw3.h>

#include "cpu.hpp"

extern const char *PROGRAM_NAME;

extern const char *VERTEX_SHADER_FILE;
extern const char *FRAGMENT_SHADER_FILE;

const int SCREEN_WIDTH = 160;
const int SCREEN_HEIGHT = 144;
const int VBLANK_HEIGHT = 10;

// There are 160 columns and 144 scanlines, as well as a 10-line
// vblank period. Every line takes 108.7 microseconds. At a CPU
// frequency of 4194304 Hz, that's about 456 clock cycles per
// scanline. (455.92, probably caused by rounding error?) So that's
// enough for... 2 cycles per column makes 320 total, with 136 left
// over?

// A random file on the internet ("GameBoy CPU Timing v0.01") confirms
// 456 cycles per line and claims 70224 cycles per frame. I'll go with
// that. Both numbers are doubled in the GBC's double-speed mode. The
// super gameboy has slightly increased CPU speed and
// correspondingly-increased sync speeds - presumably they're the same
// number of cycles.

// REG_LCD_CONTROL (LCDC) has 8 flags.
// Bit 0: BG display (ignored in GBC)
// Bit 1: Sprite display
// Bit 2: Sprite size flag
// Bit 3: BG code area selection flag
//        0: 0x9800-0x9bff
//        1: 0x9c00-0x0fff
// Bit 4: BG character data selection flag
//        0: 0x8800-0x97ff (signed indices)
//        1: 0x8000-0x8fff (unsigned indices)
// Bit 5: Windowing on/off
// Bit 6: Window code area selection flag
//        0: 0x9800-0x9bff
//        1: 0x9c00-0x0fff
// Bit 7: LCD on/off

// code area selection must be analogous the the NES's nametables, and
// character data selection to the pattern tables?

// So the background tile map (must be the same as "code area"?) is 32
// rows of 32 bytes each. Each byte is a tile number (signed or
// unsigned index into the character data region).

const int SPRITE_LIMIT = 10;
const int SPRITE_X_OFFSET = 8;
const int SPRITE_Y_OFFSET = 16;

const uint8_t LCDC_BG_DISPLAY = 1<<0;
const uint8_t LCDC_SPRITE_DISPLAY = 1<<1;
const uint8_t LCDC_SPRITE_SIZE = 1<<2;
const uint8_t LCDC_BG_CODE = 1<<3;
const uint8_t LCDC_BG_CHR = 1<<4;
const uint8_t LCDC_WINDOW_DISPLAY = 1<<5;
const uint8_t LCDC_WINDOW_CODE = 1<<6;
const uint8_t LCDC_DISPLAY = 1<<7;

const uint8_t SPRITE_COLOR = 0xf; // color mode
const uint8_t SPRITE_PALETTE = 1<<4; // non-color mode
const uint8_t SPRITE_FLIP_H = 1<<5;
const uint8_t SPRITE_FLIP_V = 1<<6;
const uint8_t SPRITE_PRIORITY = 1<<7;

// also handling controller state here, since GLFW does that
const uint8_t JOYPAD_DIRECTIONS = 1<<4; // port P14
const uint8_t JOYPAD_BUTTONS = 1<<5; // port P15
const uint8_t JOYPAD_RIGHT = 1<<0; // port P10
const uint8_t JOYPAD_A = 1<<0; // port P10
const uint8_t JOYPAD_LEFT = 1<<1; // port P11
const uint8_t JOYPAD_B = 1<<1; // port P11
const uint8_t JOYPAD_UP = 1<<2; // port P12
const uint8_t JOYPAD_SELECT = 1<<2; // port P12
const uint8_t JOYPAD_DOWN = 1<<3; // port P13
const uint8_t JOYPAD_START = 1<<3; // port P13

typedef struct vertex {
  float x;
  float y;
} vertex;

class CPU;

class Screen {
public:
  // not going to figure out all of C++'s various named-argument
  // idioms right now - this will work for now
  Screen(CPU *c, bool vsyncParam=true, bool displayTiles=false);
  void draw();
  uint8_t getKeys(uint8_t inputFlags);
private:
  GLFWwindow *window;
  GLuint shader;
  // shader attribute locations
  GLint posAttrib;
  // shader uniform locations
  GLint texUniform;
  // buffers
  GLuint bgVao;
  GLuint bgVbo;
  // textures
  GLuint texName;

  bool vsync;

  void drawMainWindow();
  void drawBackground(float *pixels);
  void drawSprites(float *pixels);
  void drawWindow(float *pixels);

  void initShaders();

  // got weird dependency issues trying to make this a reference.
  // probably fixable.
  CPU *cpu;

  // stuff for debug windows
  GLFWwindow *tileWindow;
  void initTileWindow();
  void drawTileWindow();

  GLuint tileVao;
  GLuint tileVbo;
  GLuint tileWindowTexName;
};

int initWindow(GLFWwindow**);

void die();

#endif // #ifndef SCREEN_H
