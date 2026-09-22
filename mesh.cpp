#pragma once
#include <math.h>
#include <stdlib.h>

#include "color.h"
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
           std::vector<color> c, std::vector<int> i)
    : vertices(v), normals(n), vertexColors(c), indices(i) {}

std::vector<glm::vec3> mesh::getVertices() { return vertices; }
std::vector<glm::vec3> mesh::getNormals() { return normals; }
std::vector<int> mesh::getIndices() { return indices; }
material::material(color a, bool m, float r, float e, float t, float i)
    : albedo(a), metallic(m), roughness(r), emission(e), transmission(t),
      ior(i) {}

std::vector<mesh> scene;
static void addmesh(mesh m) { scene.push_back(m); }
static mesh getMesh(int id) { return scene[id]; }