#pragma once
#include <math.h>
#include <stdlib.h>

#include <glm/glm.hpp>

struct mesh {
  std::vector<glm::vec3> vertices;
  std::vector<glm::vec3> normals;
  std::vector<int> indices;

  mesh(std::vector<glm::vec3> v, std::vector<glm::vec3> n, std::vector<int> i);
  std::vector<glm::vec3> getVertices();
  std::vector<glm::vec3> getNormals();
  std::vector<int> getIndices();
};

static void addmesh(mesh m);
static mesh getMesh(int id);
