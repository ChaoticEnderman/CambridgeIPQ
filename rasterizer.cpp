#include <algorithm>
#include <glm/common.hpp>
#include <glm/ext/vector_int2.hpp>
#include <stdlib.h>

#include <algorithm>
#include <glm/glm.hpp>
#include <iostream>

#include "camera.h"
#include "renderer.h"

void raster2DLine(glm::ivec2 v1, glm::ivec2 v2) {
  drawLine(v1.x, v1.y, v2.x, v2.y);
}

void raster2DTriangle(glm::ivec2 vertices[], int r, int g, int b) {
  glm::ivec2 p;
  // Finding vectors from each vertices to each other
  glm::ivec2 n1 = vertices[1] - vertices[0];
  glm::ivec2 n2 = vertices[2] - vertices[1];
  glm::ivec2 n3 = vertices[0] - vertices[2];
  // Rotate the vectors to get the normals
  int temp = n1.x;
  n1.x = -n1.y;
  n1.y = temp;
  temp = n2.x;
  n2.x = -n2.y;
  n2.y = temp;
  temp = n3.x;
  n3.x = -n3.y;
  n3.y = temp;
  // Find the bounding box of the triangle
  glm::ivec2 topLeft =
      glm::ivec2(std::min({vertices[0].x, vertices[1].x, vertices[2].x, 0}),
                 std::min({vertices[0].y, vertices[1].y, vertices[2].y, 0}));
  glm::ivec2 bottomRight =
      glm::ivec2(std::max({vertices[0].x, vertices[1].x, vertices[2].x,
                           getScreenResolution().x}),
                 std::max({vertices[0].y, vertices[1].y, vertices[2].y,
                           getScreenResolution().y}));
  setColor(r, g, b);
  // Loop through all pixels inside the bounding box
  for (int i = topLeft.x; i < bottomRight.x; i++) {
    for (int j = topLeft.y; j < bottomRight.y; j++) {
      p = glm::ivec2(i, j);
      // If the vector is generally in the general same direction of each
      // normals, for all three normals, render it
      if (glm::dot(glm::vec2(n1), glm::vec2(p - vertices[0])) > 0 &&
          glm::dot(glm::vec2(n2), glm::vec2(p - vertices[1])) > 0 &&
          glm::dot(glm::vec2(n3), glm::vec2(p - vertices[2])) > 0) {
        drawPoint(i, j);
      }
    }
  }
}

void rasterPoint(glm::vec3 vertices) {
  // glm::vec3 translation = getCameraPosition() - vertices;
}