#pragma once
#include <cstdint>

struct color {
  uint8_t r, g, b, a;

  color(uint8_t R, uint8_t G, uint8_t B) {
    r = R;
    g = G;
    b = B;
    a = 255;
  }

  color(uint8_t R, uint8_t G, uint8_t B, uint8_t A) {
    r = R;
    g = G;
    b = B;
    a = A;
  }

  enum class base {
    black,
    white,
    red,
    green,
    blue,
    yellow,
    cyan,
    magenta,
  };

  color(base color) {
    switch (color) {
    case base::black:
      r = 0;
      g = 0;
      b = 0;
      a = 255;
      break;
    case base::white:
      r = 255;
      g = 255;
      b = 255;
      a = 255;
      break;
    case base::red:
      r = 255;
      g = 0;
      b = 0;
      a = 255;
      break;
    case base::green:
      r = 0;
      g = 255;
      b = 0;
      a = 255;
      break;
    case base::blue:
      r = 0;
      g = 0;
      b = 255;
      a = 255;
      break;
    case base::yellow:
      r = 255;
      g = 255;
      b = 0;
      a = 255;
      break;
    case base::cyan:
      r = 0;
      g = 255;
      b = 255;
      a = 255;
      break;
    case base::magenta:
      r = 255;
      g = 0;
      b = 255;
      a = 255;
      break;
    }
  }
};
