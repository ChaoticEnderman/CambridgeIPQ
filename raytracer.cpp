#include <algorithm>
#include <atomic>
#include <cstddef>
#include <glm/common.hpp>
#include <glm/ext/matrix_float3x3.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_int2.hpp>
#include <stdlib.h>

#include <glm/glm.hpp>
#include <iostream>
#include <sys/types.h>
#include <vector>

#include "camera.h"
#include "color.h"
#include "mesh.h"
#include "renderer.h"

struct Ray {
  glm::vec3 position;
  glm::vec3 direction;
  std::uint8_t depth;
  Ray(glm::vec3 _position, glm::vec3 _direction, int _depth) {
    position = _position;
    direction = _direction;
    depth = _depth;
  };
};

bool rayTriangleIntersect() { return false; }

color traceRay() {
  bool intersect = false;
  std::vector<mesh> scene2;
  for (int i = 0; i < scene2.size(); i++) {
    const int triangleCount = scene2[i].indices.size() / 3;
    for (int j = 0; j < triangleCount; i++) {
      if (rayTriangleIntersect()) {
        return color::base::white;
      } else {
        return color::base::black;
      }
    }
  };
  return color::base::black;
}

void traceRays() {
  color frameBuffer[getScreenResolution().x][getScreenResolution().y];
  for (int i = 0; i < getScreenResolution().x; i++) {
    for (int j = 0; j < getScreenResolution().y; j++) {
      frameBuffer[i][j] = traceRay();
    }
  }
}
