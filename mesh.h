#pragma once
#include <glm/glm.hpp>
#include <math.h>
#include <stdlib.h>
#include <vector>

#include "color.h"

struct material {
  color albedo;
  bool metallic;
  float roughness;
  float emission;
  float transmission;
  float ior;

  color getAlbedo();
  bool getMetallic();
  float getRoughness();
  float getEmission();
  float getTransmission();
  float getIOR();

  enum class base {
    redFill,
    greenFill,
    blueFill,
    silver,
    gold,
    mirror,
    water,
    glassFrosted,
    glassPure,
    concrete,
    wood,
    laminate
  };
  material(color c, bool m, float r, float e, float t, float i);
  material(base material);
};

struct mesh {
  std::vector<glm::vec3> vertices;
  std::vector<glm::vec3> normals;
  std::vector<color> vertexColors;
  std::vector<int> indices;
  material mat;

  mesh(std::vector<glm::vec3> v, std::vector<glm::vec3> n, std::vector<color> c,
       std::vector<int> i, material m);
  std::vector<glm::vec3> getVertices();
  std::vector<glm::vec3> getNormals();
  std::vector<color> getVertexColors();
  std::vector<int> getIndices();
};

// std::vector<mesh> scene;
// static void addmesh(mesh m) { scene.push_back(m); }
// static mesh getMesh(int id) { return scene[id]; }