#include <math.h>
#include <stdlib.h>

#include "mesh.h"
#include <glm/glm.hpp>
#include <vector>

// struct mesh {
//   std::vector<glm::vec3> vertices;
//   std::vector<glm::vec3> normals;
//   std::vector<int> indices;

//   mesh(std::vector<glm::vec3> v, std::vector<glm::vec3> n, std::vector<int>
//   i) {
//     vertices = v;
//     normals = n;
//     indices = i;
//   }
//   std::vector<glm::vec3> getVertices() { return vertices; }
//   std::vector<glm::vec3> getNormals() { return normals; }
//   std::vector<int> getIndices() { return indices; }
// };

mesh::mesh(std::vector<glm::vec3> v, std::vector<glm::vec3> n,
           std::vector<int> i)
    : vertices(v), normals(n), indices(i) {}

std::vector<glm::vec3> mesh::getVertices() { return vertices; }
std::vector<glm::vec3> mesh::getNormals() { return normals; }
std::vector<int> mesh::getIndices() { return indices; }

std::vector<mesh> scene;
static void addmesh(mesh m) { scene.push_back(m); }
static mesh getMesh(int id) { return scene[id]; }