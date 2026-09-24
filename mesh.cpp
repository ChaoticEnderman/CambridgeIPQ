#pragma once
#include <math.h>
#include <stdlib.h>

#include "color.h"
#include "mesh.h"
#include <glm/glm.hpp>
#include <vector>

using namespace glm;

// struct mesh {
//   std::vector<vec3> vertices;
//   std::vector<vec3> normals;
//   std::vector<int> indices;

//   mesh(std::vector<vec3> v, std::vector<vec3> n, std::vector<int>
//   i) {
//     vertices = v;
//     normals = n;
//     indices = i;
//   }
//   std::vector<vec3> getVertices() { return vertices; }
//   std::vector<vec3> getNormals() { return normals; }
//   std::vector<int> getIndices() { return indices; }
// };

material::material(color a, bool m, float r, float e, float t, float i)
    : albedo(a), metallic(m), roughness(r), emission(e), transmission(t),
      ior(i) {}
material::material(base material) {
  switch (material) {
  case base::redFill:
    albedo = color(255, 0, 0);
    metallic = false;
    roughness = 0.0f;
    emission = 0.0f;
    transmission = 0.0f;
    ior = 1.0f;
    break;
  case base::greenFill:
    albedo = color(0, 255, 0);
    metallic = false;
    roughness = 0.0f;
    emission = 0.0f;
    transmission = 0.0f;
    ior = 1.0f;
    break;
  case base::blueFill:
    albedo = color(0, 255, 255);
    metallic = false;
    roughness = 0.0f;
    emission = 0.0f;
    transmission = 0.0f;
    ior = 1.0f;
    break;
  case base::silver:
    albedo = color(90, 90, 90);
    metallic = true;
    roughness = 0.0f;
    emission = 0.0f;
    transmission = 0.0f;
    ior = 1.0f;
    break;
  case base::gold:
    albedo = color(192, 156, 48);
    metallic = true;
    roughness = 0.0f;
    emission = 0.0f;
    transmission = 0.0f;
    ior = 1.0f;
    break;
  case base::mirror:
    albedo = color(255, 255, 255);
    metallic = true;
    roughness = 0.0f;
    emission = 0.0f;
    transmission = 0.0f;
    ior = 1.0f;
    break;
  case base::water:
    albedo = color(0, 255, 255);
    metallic = true;
    roughness = 0.0f;
    emission = 0.0f;
    transmission = 0.7f;
    ior = 1.33f;
    break;
  case base::glassFrosted:
    albedo = color(200, 200, 200);
    metallic = true;
    roughness = 0.5f;
    emission = 0.0f;
    transmission = 0.6f;
    ior = 1.5f;
    break;
  case base::glassPure:
    albedo = color(200, 200, 200);
    metallic = true;
    roughness = 0.0f;
    emission = 0.0f;
    transmission = 0.9f;
    ior = 1.5f;
    break;
  case base::concrete:
    albedo = color(140, 140, 140);
    metallic = true;
    roughness = 0.3f;
    emission = 0.0f;
    transmission = 0.0f;
    ior = 1.0f;
    break;
  case base::wood:
    albedo = color(67, 40, 24);
    metallic = true;
    roughness = 0.1f;
    emission = 0.0f;
    transmission = 0.0f;
    ior = 1.0f;
    break;
  case base::laminate:
    albedo = color(67, 40, 24);
    metallic = true;
    roughness = 0.3f;
    emission = 0.0f;
    transmission = 0.0f;
    ior = 1.0f;
    break;
  }
}

mesh::mesh(std::vector<vec3> v, std::vector<vec3> n, std::vector<color> c,
           std::vector<int> i, material m)
    : vertices(v), normals(n), vertexColors(c), indices(i), mat(m) {}

std::vector<vec3> mesh::getVertices() { return vertices; }
std::vector<vec3> mesh::getNormals() { return normals; }
std::vector<int> mesh::getIndices() { return indices; }

std::vector<color> mesh::getVertexColors() { return vertexColors; }

color material::getAlbedo() { return albedo; }
bool material::getMetallic() { return metallic; }
float material::getRoughness() { return roughness; }
float material::getEmission() { return emission; }
float material::getTransmission() { return transmission; }
float material::getIOR() { return ior; }

std::vector<mesh> scene;
void addMesh(mesh m) { scene.push_back(m); }
mesh getMesh(int id) { return scene[id]; }
