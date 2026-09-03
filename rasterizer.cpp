#include <camera.h>
#include <renderer.h>
#include <stdlib.h>

#include <glm/glm.hpp>

void raster2DTriangle(glm::vec2 vertices[]) {}

void rasterPoint(glm::vec3 vertices) {
  glm::vec3 translation = getCameraPosition() - vertices;
}