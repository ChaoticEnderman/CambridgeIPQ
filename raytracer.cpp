#include <SDL2/SDL_render.h>
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
  dir = cameraMatRT * dir;
  Ray ray = Ray(getCameraPosition(), dir, 0);
  return ray;
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

int totalNS = 0;
int maxNS = 0;
ivec2 maxNSid = ivec2(0, 0);

color traceRay(Ray r, int i, int j) {
  std::chrono::time_point start = std::chrono::steady_clock::now();

  bool intersect = false;
  int objectIndex;
  int triangleIndex;
  mesh m = getMesh(0);
  // keep a copy for efficient memory
  std::vector<vec3> vertices;
  std::vector<int> indices;
  std::vector<color> colors;
  vec3 vert[3];
  color col = color::base::black;
  int triangleCount;
  for (int i = 0; i < getSceneSize(); i++) {
    triangleCount = getMesh(i).indices.size() / 3;
    m = getMesh(i);
    vertices = m.getVertices();
    indices = m.getIndices();
    colors = m.getVertexColors();
    for (int j = 0; j < triangleCount; j++) {
      vert[0] = vertices[indices[3 * j]];
      vert[1] = vertices[indices[3 * j + 1]];
      vert[2] = vertices[indices[3 * j + 2]];
      float t, u, v, w;
      bool res = rayTriangleIntersect(r, vert, &t, &u, &v, &w);
      if (res) {
        // std::cout << "intersect " << r.toString() << " " //
        //           << vert[0].x << " " << vert[0].y << " " << vert[0].z << " "
        //           << vert[1].x << " " << vert[1].y << " " << vert[1].z << " "
        //           << vert[2].x << " " << vert[2].y << " " << vert[2].z << " "
        //           << std::endl;
        // intersect = true;
        col = barycentricInterpolation(colors[3 * j], colors[3 * j + 1],
                                       colors[3 * j + 2], u, v, w);
        // std::cout << "bary " << col.toString() << colors[3 * j].toString()
        //           << colors[3 * j + 1].toString()
        //           << colors[3 * j + 2].toString() << std::endl;
      }
    }
  };

  std::chrono::time_point end = std::chrono::steady_clock::now();
  std::chrono::nanoseconds duration =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
  totalNS += duration.count();
  if (maxNS < duration.count()) {
    maxNS = duration.count();
    maxNSid = ivec2(i, j);
  }

  return col;
}

void traceRays() {
  totalNS = 0;
  maxNS = 0;
  for (int i = 0; i < getScreenResolutionX(); i++) {
    for (int j = 0; j < getScreenResolutionY(); j++) {
      Ray r = getCameraRay(vec2(i, j));
      fragment[(getScreenResolutionY() - j) * getScreenResolutionX() + i] =
          toUint32ARGB(traceRay(r, i, j));
    }
  }
  std::cout << "average ray time " << totalNS << " "
            << totalNS / getScreenResolutionX() / getScreenResolutionY()
            << " coords " << maxNSid.x << " " << maxNSid.y << " points "
            << pointIndex << std::endl;
  std::cout << "max ray time " << maxNS << std::endl;
}
