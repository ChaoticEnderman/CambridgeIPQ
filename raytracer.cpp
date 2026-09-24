#include <algorithm>
#include <atomic>
#include <cmath>
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

using namespace glm;

struct Ray {
  vec3 position;
  vec3 direction;
  std::uint8_t depth;
  float nearest;
  Ray(vec3 p, vec3 d, int e) {
    position = p;
    direction = d;
    depth = e;
    nearest = INFINITY;
  };
};

Ray getCameraRay(vec2) { return Ray(vec3(0, 0, 0), vec3(0, 0, 1), 0); }

bool rayTriangleIntersect(Ray r, vec3 v[]) {
  // float e1 = v[1] - v[0];
  // float e2 = v[2] - v[0];
}

color traceRay(Ray r) {
  bool intersect = false;
  int objectIndex;
  int triangleIndex;
  std::vector<mesh> scene2;
  vec3 vertices[3];
  mesh *m;
  for (int i = 0; i < scene2.size(); i++) {
    const int triangleCount = scene2[i].indices.size() / 3;
    for (int j = 0; j < triangleCount; i++) {
      m = &scene2[i];
      vertices[0] = scene2[i].getVertices()[scene2[i].getIndices()[3 * j]];
      vertices[1] = scene2[i].getVertices()[scene2[i].getIndices()[3 * j + 1]];
      vertices[2] = scene2[i].getVertices()[scene2[i].getIndices()[3 * j + 2]];
      if (rayTriangleIntersect(r, vertices)) {
        return color::base::white;
      }
    }
  };
  return color::base::black;
}

void traceRays() {
  color frameBuffer[getScreenResolution().x][getScreenResolution().y];
  for (int i = 0; i < getScreenResolution().x; i++) {
    for (int j = 0; j < getScreenResolution().y; j++) {
      Ray r = getCameraRay(vec2(i, j));
      frameBuffer[i][j] = traceRay(r);
    }
  }
}
