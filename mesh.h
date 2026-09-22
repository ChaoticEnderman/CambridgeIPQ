#pragma once
#include <glm/glm.hpp>
#include <math.h>
#include <stdlib.h>
#include <vector>

#include "color.h"

struct mesh {
  std::vector<glm::vec3> vertices;
  std::vector<glm::vec3> normals;
  std::vector<color> vertexColors;
  std::vector<int> indices;

  mesh(std::vector<glm::vec3> v, std::vector<glm::vec3> n, std::vector<color> c,
       std::vector<int> i);
  std::vector<glm::vec3> getVertices();
  std::vector<glm::vec3> getNormals();
  std::vector<color> getVertexColors();
  std::vector<int> getIndices();
};
struct material {
  color albedo;
  bool metallic;
  float roughness;
  float emission;
  float transmission;
  float ior;
  material(color c, bool m, float r, float e, float t, float i);
  color getColor();
  bool getMetallic();
  float getRoughness();
  float getEmission();
  float getTransmission();
  float getIOR();
};

static void addmesh(mesh m);
static mesh getMesh(int id);
