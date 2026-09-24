#include <algorithm>
#include <cmath>
#include <glm/common.hpp>
#include <glm/ext/matrix_float2x2.hpp>
#include <glm/ext/matrix_float3x3.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_int2.hpp>
#include <ostream>
#include <stdlib.h>

#include <chrono>
#include <glm/glm.hpp>
#include <iostream>
#include <vector>

#include "camera.h"
#include "mesh.h"
#include "renderer.h"

using namespace glm;

float camLen = 0.1f;
constexpr int resX = getScreenResolutionX();
constexpr int resY = getScreenResolutionY();
float zBuffer[resX][resY];

mat4x4 cameraMat = mat4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

inline float cross2D(ivec2 a, ivec2 b) { return a.x * b.y - a.y * b.x; }

void raster2DLine(ivec2 v1, ivec2 v2) {
  drawLine(v1.x, getScreenResolution().y - v1.y, v2.x,
           getScreenResolution().y - v2.y);
}

void raster3DLine(vec3 v1, vec3 v2) {
  ivec2 vs1, vs2;
  v1 = v1 - getCameraPosition();
  v1 = cameraMat * vec4(v1, 1);
  v2 = v2 - getCameraPosition();
  v2 = cameraMat * vec4(v2, 1);
  vec2 temp = vec2(v1.x / v1.z * camLen, v1.y / v1.z * camLen);
  temp.x *= getScreenResolution().x;
  temp.y *= getScreenResolution().y;
  temp.x += getScreenResolution().x >> 1;
  temp.y += getScreenResolution().y >> 1;
  vs1 = temp;
  temp = vec2(v2.x / v2.z * camLen, v2.y / v2.z * camLen);
  temp.x *= getScreenResolution().x;
  temp.y *= getScreenResolution().y;
  temp.x += getScreenResolution().x >> 1;
  temp.y += getScreenResolution().y >> 1;
  vs2 = temp;
  std::cout << "2d line " << vs1.x << " " << vs1.y << " " << vs2.x << " "
            << vs2.y << std::endl;
  raster2DLine(vs1, vs2);
}
void raster3Dcoord() {
  setColor(color::base::white);
  raster3DLine(vec3(5, 0, 0), vec3(-5, 0, 0));
  raster3DLine(vec3(0, 5, 0), vec3(0, -5, 0));
  raster3DLine(vec3(0, 0, 5), vec3(0, 0, -5));
}

// taking optional z depth of each pixels for depth buffer
void raster2DTriangle(ivec2 vertices[], color c,
                      std::array<float, 3> depth = {0.0f, 0.0f, 0.0f}) {
  // Find the bounding box of the triangle
  ivec2 topLeft = ivec2(
      std::max({std::min({vertices[0].x, vertices[1].x, vertices[2].x}), 0}),
      std::max({std::min({vertices[0].y, vertices[1].y, vertices[2].y}), 0}));
  ivec2 bottomRight =
      ivec2(std::min({std::max({vertices[0].x, vertices[1].x, vertices[2].x}),
                      getScreenResolution().x}),
            std::min({std::max({vertices[0].y, vertices[1].y, vertices[2].y}),
                      getScreenResolution().x}));
  setColor(c);
  // raster2DLine(vertices[0], vertices[1]);
  // raster2DLine(vertices[1], vertices[2]);
  // raster2DLine(vertices[2], vertices[0]);

  // return;
  //  Loop through all pixels inside the bounding box
  ivec2 points[getScreenResolution().x * getScreenResolution().y];
  int size = 0;

  // vectors
  ivec2 ab = vertices[1] - vertices[0];
  ivec2 bc = vertices[2] - vertices[1];
  ivec2 ca = vertices[0] - vertices[2];
  ivec2 ap = topLeft - vertices[0];
  ivec2 bp = topLeft - vertices[1];
  ivec2 cp = topLeft - vertices[2];

  // area for calculating barycentric weight in one step
  float area = cross2D(vertices[1] - vertices[0], vertices[2] - vertices[0]);

  // weights of each parts unnormalized
  float startLambdaA = cross2D(bc, bp) / area;
  float startLambdaB = cross2D(ca, cp) / area;
  float startLambdaC = cross2D(ab, ap) / area;
  float startZDepth = startLambdaA * depth[0] + startLambdaB * depth[1] +
                      startLambdaC * depth[2];
  float deltaLambdaAx = cross2D(bc, bp + ivec2(1, 0)) / area - startLambdaA;
  float deltaLambdaBx = cross2D(ca, cp + ivec2(1, 0)) / area - startLambdaB;
  float deltaLambdaCx = cross2D(ab, ap + ivec2(1, 0)) / area - startLambdaC;
  float deltaLambdaAy = cross2D(bc, bp + ivec2(0, 1)) / area - startLambdaA;
  float deltaLambdaBy = cross2D(ca, cp + ivec2(0, 1)) / area - startLambdaB;
  float deltaLambdaCy = cross2D(ab, ap + ivec2(0, 1)) / area - startLambdaC;
  float deltaZDepthx = (startLambdaA + deltaLambdaAx) * depth[0] +
                       (startLambdaB + deltaLambdaBx) * depth[1] +
                       (startLambdaC + deltaLambdaCx) * depth[2] - startZDepth;
  float deltaZDepthy = (startLambdaA + deltaLambdaAy) * depth[0] +
                       (startLambdaB + deltaLambdaBy) * depth[1] +
                       (startLambdaC + deltaLambdaCy) * depth[2] - startZDepth;

  std::chrono::time_point start = std::chrono::steady_clock::now();
  std::chrono::time_point end = std::chrono::steady_clock::now();

  for (int i = topLeft.x; i < bottomRight.x; i++) {
    float innerLambdaA = startLambdaA, innerLambdaB = startLambdaB,
          innerLambdaC = startLambdaC;
    float innerZDepth = startZDepth;
    for (int j = topLeft.y; j < bottomRight.y; j++) {
      if (innerLambdaA > 0.0f && innerLambdaB > 0.0f && innerLambdaC > 0.0f &&
          innerZDepth < zBuffer[i][j]) {
        if (innerZDepth > 0 && innerZDepth < zBuffer[i][j]) {
          points[size] = ivec2(i, getScreenResolutionY() - j);
          size++;
          zBuffer[i][j] = innerZDepth;
        }
      }
      innerLambdaA += deltaLambdaAy;
      innerLambdaB += deltaLambdaBy;
      innerLambdaC += deltaLambdaCy;
      innerZDepth += deltaZDepthy;
    }
    startLambdaA += deltaLambdaAx;
    startLambdaB += deltaLambdaBx;
    startLambdaC += deltaLambdaCx;
    startZDepth += deltaZDepthx;
  }
  end = std::chrono::steady_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  // std::cout << "loop elapsed: " << duration.count() << " us\n";

  // ivec2 p = ivec2(0, 0);
  // for (int i = topLeft.x; i < bottomRight.x; i++) {
  //   for (int j = topLeft.y; j < bottomRight.y; j++) {
  //     p = ivec2(i, j);
  //     // If the vector is generally in the general same direction of each
  //     // normals, for all three normals, render it
  //     if (dot(vec2(n1), vec2(p - vertices[0])) > 0 &&
  //         dot(vec2(n2), vec2(p - vertices[1])) > 0 &&
  //         dot(vec2(n3), vec2(p - vertices[2])) > 0) {
  //       points[size] = (ivec2(i, getScreenResolution().y - j));
  //       size++;
  //     }
  //   }
  // }

  drawPoints(points, size);
  // std::cout << "Elapsed: " << duration.count() << " us\n";
}

mat2x2 aspect = mat2x2(0.5625, 0, 0, 1);

void raster3DTriangle(vec3 vertices[], color c) {
  ivec2 mappedVertices[3] = {};
  int discard = 0;
  for (int i : {0, 1, 2}) {
    if (vertices[i].z < camLen) {
      discard++;
    }
  }
  if (discard > 1) {
    return;
  }
  for (int i : {0, 1, 2}) {
    // undo translation
    // vertices[i] = vertices[i] - getCameraPosition();
    // undo yaw
    // vertices[i] = yawMat * vertices[i];
    // undo pitch
    // vertices[i] = pitchMat * vertices[i];

    // undo translation and yaw and pitch
    vertices[i] = cameraMat * vec4(vertices[i], 1);

    if (vertices[i].z < camLen) {
      return;
    }

    vec2 temp = vec2(vertices[i].x / vertices[i].z * camLen,
                     vertices[i].y / vertices[i].z * camLen);
    temp.x *= getScreenResolution().x;
    temp.y *= getScreenResolution().y * 1.78; // approx 16:9 aspect ratio
    temp.x += getScreenResolution().x >> 1;
    temp.y += getScreenResolution().y >> 1;
    mappedVertices[i] = temp;
  }
  raster2DTriangle(
      mappedVertices, c,
      std::array<float, 3>{vertices[0].z, vertices[1].z, vertices[2].z});
}

// number of indices should be exactly three times the number of count
void raster3DMesh(mesh m) {
  for (int i = 0; i < m.getIndices().size() / 3; i++) {
    vec3 vertices3d[3];
    vertices3d[0] = m.getVertices()[m.getIndices()[3 * i]];
    vertices3d[1] = m.getVertices()[m.getIndices()[3 * i + 1]];
    vertices3d[2] = m.getVertices()[m.getIndices()[3 * i + 2]];
    raster3DTriangle(vertices3d, m.getVertexColors()[3 * i]);
  }
}

void computeRotationMatrixes() {
  float pitch = -getCameraRotation().x;
  float yaw = -getCameraRotation().y;

  mat4x4 transMat =
      mat4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -getCameraPosition().x,
             -getCameraPosition().y, -getCameraPosition().z, 1);
  mat3x3 pitchMat =
      mat3x3(1, 0, 0, 0, cos(pitch), sin(pitch), 0, -sin(pitch), cos(pitch));
  mat3x3 yawMat =
      mat3x3(cos(yaw), 0, -sin(yaw), 0, 1, 0, sin(yaw), 0, cos(yaw));
  cameraMat = pitchMat * yawMat;
  cameraMat *= transMat;
}

void clearZBuffer() {
  for (int i = 0; i < resX; i++) {
    for (int j = 0; j < resY; j++) {
      zBuffer[i][j] = 1000;
    }
  }
}

void rasterPoint(vec3 vertices) {
  // vec3 translation = getCameraPosition() - vertices;
}