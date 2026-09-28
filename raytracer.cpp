#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <glm/common.hpp>
#include <glm/ext/matrix_float3x3.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_int2.hpp>
#include <glm/geometric.hpp>
#include <iterator>
#include <stdlib.h>

#include <glm/glm.hpp>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <vector>

#include "camera.h"
#include "color.h"
#include "mesh.h"
#include "renderer.h"

using namespace glm;

color frameBuffer[getScreenResolutionX()][getScreenResolutionY()];
ivec2 pointBuffer[getScreenResolutionX() * getScreenResolutionY()];
int pointIndex;

mat3x3 cameraMatRT;

struct Ray {
  vec3 origin;
  vec3 direction;
  std::uint8_t depth;
  float nearest;
  Ray(vec3 o, vec3 d, int e) {
    origin = o;
    direction = d;
    depth = e;
    nearest = INFINITY;
  };
  std::string toString() {
    return std::to_string((origin.x)) + " " + std::to_string((origin.y)) + " " +
           std::to_string((origin.z)) + " " + std::to_string((direction.x)) +
           " " + std::to_string((direction.y)) + " " +
           std::to_string((direction.z));
  };
};

void computeCameraMatrixes() {
  float pitch = getCameraRotation().x;
  float yaw = getCameraRotation().y;

  mat3x3 yawMat =
      mat3x3(cos(yaw), 0, -sin(yaw), 0, 1, 0, sin(yaw), 0, cos(yaw));
  mat3x3 pitchMat =
      mat3x3(1, 0, 0, 0, cos(pitch), sin(pitch), 0, -sin(pitch), cos(pitch));

  cameraMatRT = yawMat * pitchMat;
}

Ray getCameraRay(vec2 point) {
  // normalize to (-1, 1) range
  vec3 dir = vec3(point.x / getScreenResolutionX() * 2.0f - 1.0f,
                  point.y / getScreenResolutionX() * 2.0f - 1.0f, 1.0f);
  dir = normalize(dir);
  return Ray(getCameraPosition(), cameraMatRT * dir, 0);
}

bool rayTriangleIntersect(Ray r, vec3 vert[], float *t, float *u, float *v,
                          float *w) {
  // Möller–Trumbore paper
  // Fast Minimum Storage Ray/Triangle Intersection
  // https://cadxfem.org/inf/Fast%20MinimumStorage%20RayTriangle%20Intersection.pdf
  float epsilon = 0.0001;
  vec3 e1 = vert[1] - vert[0];
  vec3 e2 = vert[2] - vert[0];
  // calculating determinant
  vec3 pvec = cross(r.direction, e2);
  float det = dot(e1, pvec);

  // if determinant near zero, ray is near parallel
  if (det < epsilon) {
    return false;
  }
  vec3 tvec = r.origin - vert[0];
  // find u and v
  *u = dot(tvec, pvec);
  if (*u < 0.0 || *u > det) {
    // barycentric coordinates should be 0-1 for points inside triangle
    return false;
  }
  vec3 qvec = cross(tvec, e1);
  *v = dot(r.direction, qvec);
  if (*v < 0.0 || *u + *v > det) {
    // barycentric coordinates should be 0-1 for points inside triangle
    return false;
  }
  // normalize
  float inverseDet = 1.0 / det;
  *t = dot(e2, qvec) * inverseDet;
  *u *= inverseDet;
  *v *= inverseDet;
  // get w from u and v
  *w = 1.0f - *u - *v;
  return true;
}

color traceRay(Ray r) {
  std::chrono::time_point start = std::chrono::steady_clock::now();

  bool intersect = false;
  int objectIndex;
  int triangleIndex;
  mesh m = getMesh(0);
  // keep a copy for efficient memory
  std::vector<vec3> vertices;
  std::vector<int> indices;
  vec3 vert[3];
  int triangleCount;
  for (int i = 0; i < getSceneSize(); i++) {
    triangleCount = getMesh(i).indices.size() / 3;
    for (int j = 0; j < triangleCount; j++) {
      m = getMesh(i);
      vertices = m.getVertices();
      indices = m.getIndices();
      vert[0] = vertices[indices[3 * j]];
      vert[1] = vertices[indices[3 * j + 1]];
      vert[2] = vertices[indices[3 * j + 2]];
      float t, u, v, w;
      bool res = rayTriangleIntersect(r, vert, &t, &u, &v, &w);
      if (res) {
        /*std::cout << "intersect " << r.toString() << " " //
                  << vert[0].x << " " << vert[0].y << " " << vert[0].z << " "
                  << vert[1].x << " " << vert[1].y << " " << vert[1].z << " "
                  << vert[2].x << " " << vert[2].y << " " << vert[2].z << " "
                  << std::endl;
        intersect = true;*/
        return color::base::white;
      }
    }
  };

  std::chrono::time_point end = std::chrono::steady_clock::now();
  std::chrono::nanoseconds duration =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
  std::cout << "Ray time: " << duration.count() << " ns\n";

  return color::base::black;
}

void traceRays() {
  setColor(color::base::white);
  pointIndex = 0;
  for (int i = 0; i < getScreenResolutionX(); i++) {
    for (int j = 0; j < getScreenResolutionY(); j++) {
      Ray r = getCameraRay(vec2(i, j));
      frameBuffer[i][j] = traceRay(r);
      if (colorEquals(frameBuffer[i][j], color::base::white)) {
        pointBuffer[pointIndex] = ivec2(i, -j);
        pointIndex++;
      }
    }
  }
  drawPoints(pointBuffer, pointIndex);
}
