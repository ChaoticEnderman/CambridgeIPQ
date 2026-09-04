#include <stdlib.h>

#include <glm/glm.hpp>
#include <iostream>

#include "camera.h"
#include "renderer.h"

void raster2DLine(glm::ivec2 v1, glm::ivec2 v2) {
  drawLine(v1.x, v1.y, v2.x, v2.y);
}

void raster2DTriangle(glm::ivec2 vertices[]) {
  std::cout << "raster2d" << std::endl;
  setColor(0, 127, 255, 255);
  raster2DLine(vertices[0], vertices[1]);
  raster2DLine(vertices[1], vertices[2]);
  raster2DLine(vertices[2], vertices[0]);
  glm::ivec2 p;
  for (int i = 0; i < 640; i++) {
    for (int j = 0; j < 360; j++) {
      std::cout << "loop" << std::endl;
      p = glm::ivec2(i, j);
      glm::ivec2 m1 = glm::ivec2((vertices[0].x + vertices[1].x) >> 1,
                                 (vertices[0].x + vertices[1].x) >> 1);
      glm::ivec2 m2 = glm::ivec2((vertices[1].x + vertices[2].x) >> 1,
                                 (vertices[1].x + vertices[2].x) >> 1);
      glm::ivec2 m3 = glm::ivec2((vertices[2].x + vertices[0].x) >> 1,
                                 (vertices[2].x + vertices[0].x) >> 1);
      if (glm::dot(glm::vec2(m1), glm::vec2(p)) > 0 &&
          glm::dot(glm::vec2(m2), glm::vec2(p)) > 0 &&
          glm::dot(glm::vec2(m3), glm::vec2(p)) > 0) {
        drawPoint(i, j);
      }
    }
  }
}

void rasterPoint(glm::vec3 vertices) {
  // glm::vec3 translation = getCameraPosition() - vertices;
}