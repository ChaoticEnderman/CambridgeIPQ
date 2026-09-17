#include <algorithm>
#include <glm/common.hpp>
#include <glm/ext/matrix_float3x3.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_int2.hpp>
#include <stdlib.h>

#include <chrono>
#include <glm/glm.hpp>
#include <iostream>
#include <vector>

#include "camera.h"
#include "renderer.h"
float camLen = 0.1f;

glm::mat4x4 cameraMat =
    glm::mat4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

void raster2DLine(glm::ivec2 v1, glm::ivec2 v2) {
  drawLine(v1.x, getScreenResolution().y - v1.y, v2.x,
           getScreenResolution().y - v2.y);
}

void raster3DLine(glm::vec3 v1, glm::vec3 v2) {
  glm::ivec2 vs1, vs2;
  v1 = v1 - getCameraPosition();
  v1 = cameraMat * glm::vec4(v1, 1);
  v2 = v2 - getCameraPosition();
  v2 = cameraMat * glm::vec4(v2, 1);
  glm::vec2 temp = glm::vec2(v1.x / v1.z * camLen, v1.y / v1.z * camLen);
  temp.x *= getScreenResolution().x;
  temp.y *= getScreenResolution().y;
  temp.x += getScreenResolution().x >> 1;
  temp.y += getScreenResolution().y >> 1;
  vs1 = temp;
  temp = glm::vec2(v2.x / v2.z * camLen, v2.y / v2.z * camLen);
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
  raster3DLine(glm::vec3(5, 0, 0), glm::vec3(-5, 0, 0));
  raster3DLine(glm::vec3(0, 5, 0), glm::vec3(0, -5, 0));
  raster3DLine(glm::vec3(0, 0, 5), glm::vec3(0, 0, -5));
}

void raster2DTriangle(glm::ivec2 vertices[], color c) {
  // auto start = std::chrono::steady_clock::now();
  //  Finding vectors from each vertices to each other
  glm::ivec2 vecb = vertices[1] - vertices[0];
  glm::ivec2 vecc = vertices[2] - vertices[0];
  glm::ivec2 n1;
  glm::ivec2 n2;
  glm::ivec2 n3;
  if (glm::cross(glm::vec3(vecb.x, vecb.y, 0), glm::vec3(vecc.x, vecb.y, 0)).z <
      0.0f) {
    glm::ivec2 temp = vertices[1];
    vertices[1] = vertices[2];
    vertices[2] = temp;
  }
  n1 = vertices[1] - vertices[0]; // b-a
  n2 = vertices[2] - vertices[1]; // c-b
  n3 = vertices[0] - vertices[2]; // a-c

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
  setColor(c);
  raster2DLine(vertices[0], vertices[1]);
  raster2DLine(vertices[1], vertices[2]);
  raster2DLine(vertices[2], vertices[0]);
  // return;
  //  Loop through all pixels inside the bounding box
  glm::ivec2 points[getScreenResolution().x * getScreenResolution().y >> 1];
  int size = 0;

  // optimization from 17000us to 1100us
  float startDot0 = glm::dot(glm::vec2(n1), glm::vec2(topLeft - vertices[0]));
  float startDot1 = glm::dot(glm::vec2(n2), glm::vec2(topLeft - vertices[1]));
  float startDot2 = glm::dot(glm::vec2(n3), glm::vec2(topLeft - vertices[2]));
  float deltaDot0x = n1.x * (topLeft.x + 1 - vertices[0].x) +
                     n1.y * (topLeft.y - vertices[0].y) - startDot0;
  float deltaDot1x = n2.x * (topLeft.x + 1 - vertices[1].x) +
                     n2.y * (topLeft.y - vertices[1].y) - startDot1;
  float deltaDot2x = n3.x * (topLeft.x + 1 - vertices[2].x) +
                     n3.y * (topLeft.y - vertices[2].y) - startDot2;
  float deltaDot0y = n1.x * (topLeft.x - vertices[0].x) +
                     n1.y * (topLeft.y + 1 - vertices[0].y) - startDot0;
  float deltaDot1y = n2.x * (topLeft.x - vertices[1].x) +
                     n2.y * (topLeft.y + 1 - vertices[1].y) - startDot1;
  float deltaDot2y = n3.x * (topLeft.x - vertices[2].x) +
                     n3.y * (topLeft.y + 1 - vertices[2].y) - startDot2;

  auto start = std::chrono::steady_clock::now();
  for (int i = topLeft.x; i < bottomRight.x; i++) {
    float innerDot0 = startDot0, innerDot1 = startDot1, innerDot2 = startDot2;
    for (int j = topLeft.y; j < bottomRight.y; j++) {
      // If the vector is generally in the general same direction of each
      // normals, for all three normals, render it
      if (innerDot0 > 0.0f & innerDot1 > 0.0f & innerDot2 > 0.0f) {
        points[size] = glm::ivec2(i, getScreenResolution().y - j);
        size++;
      }
      innerDot0 += deltaDot0y;
      innerDot1 += deltaDot1y;
      innerDot2 += deltaDot2y;
    }
    startDot0 += deltaDot0x;
    startDot1 += deltaDot1x;
    startDot2 += deltaDot2x;
  }

  // glm::ivec2 p = glm::ivec2(0, 0);
  // for (int i = topLeft.x; i < bottomRight.x; i++) {
  //   for (int j = topLeft.y; j < bottomRight.y; j++) {
  //     p = glm::ivec2(i, j);
  //     // If the vector is generally in the general same direction of each
  //     // normals, for all three normals, render it
  //     if (glm::dot(glm::vec2(n1), glm::vec2(p - vertices[0])) > 0 &&
  //         glm::dot(glm::vec2(n2), glm::vec2(p - vertices[1])) > 0 &&
  //         glm::dot(glm::vec2(n3), glm::vec2(p - vertices[2])) > 0) {
  //       points[size] = (glm::ivec2(i, getScreenResolution().y - j));
  //       size++;
  //     }
  //   }
  // }

  drawPoints(points, size);
  auto end = std::chrono::steady_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  // std::cout << "Elapsed: " << duration.count() << " us\n";
}

void raster3DTriangle(glm::vec3 vertices[], color c) {
  glm::ivec2 mappedVertices[3] = {};
  // for loop syntax to reverse winding order
  for (int i : {0, 1, 2}) {
    // undo translation
    // vertices[i] = vertices[i] - getCameraPosition();
    // undo yaw
    // vertices[i] = yawMat * vertices[i];
    // undo pitch
    // vertices[i] = pitchMat * vertices[i];

    // undo translation and yaw and pitch
    vertices[i] = cameraMat * glm::vec4(vertices[i], 1);

    if (vertices[i].z < camLen) {
      return;
    }

    glm::vec2 temp = glm::vec2(vertices[i].x / vertices[i].z * camLen,
                               vertices[i].y / vertices[i].z * camLen);
    temp.x *= getScreenResolution().x;
    temp.y *= getScreenResolution().y;
    temp.x += getScreenResolution().x >> 1;
    temp.y += getScreenResolution().y >> 1;
    mappedVertices[i] = temp;
  }
  raster2DTriangle(mappedVertices, c);
}

// number of indices should be exactly three times the number of count
void raster3DMesh(glm::vec3 vertices[], int indices[], int count) {
  std::cout << "count is " << count << std::endl;
  for (int i = 0; i < count; i++) {
    glm::vec3 vertices3d[3];
    vertices3d[0] = vertices[indices[3 * i]];
    vertices3d[1] = vertices[indices[3 * i + 1]];
    vertices3d[2] = vertices[indices[3 * i + 2]];
    raster3DTriangle(vertices3d, color::base::white);
  }
}

void computeRotationMatrixes() {
  float pitch = -getCameraRotation().x;
  float yaw = -getCameraRotation().y;

  glm::mat4x4 transMat =
      glm::mat4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -getCameraPosition().x,
                  -getCameraPosition().y, -getCameraPosition().z, 1);
  glm::mat3x3 pitchMat = glm::mat3x3(1, 0, 0, 0, cos(pitch), sin(pitch), 0,
                                     -sin(pitch), cos(pitch));
  glm::mat3x3 yawMat =
      glm::mat3x3(cos(yaw), 0, -sin(yaw), 0, 1, 0, sin(yaw), 0, cos(yaw));
  cameraMat = pitchMat * yawMat;
  cameraMat *= transMat;
}

void rasterPoint(glm::vec3 vertices) {
  // glm::vec3 translation = getCameraPosition() - vertices;
}