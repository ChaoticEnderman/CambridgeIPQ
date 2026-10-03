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

color barycentricInterpolation(color A, color B, color C, float u, float v,
                               float w) {
  return color((A.r * u + B.r * v + C.r * w), (A.g * u + B.g * v + C.g * w),
               (A.b * u + B.b * v + C.b * w), (A.a * u + B.a * v + C.a * w));
}

bool colorEquals(color A, color B) {
  return A.r == B.r && A.g == B.g && A.b == B.b && A.a == B.a;
}

uint32_t toUint32ARGB(color a) {
  return (uint32_t)(a.a) << 24 | (uint32_t)(a.r) << 16 | (uint32_t)(a.g) << 8 |
         (uint32_t)(a.b) << 0;
}