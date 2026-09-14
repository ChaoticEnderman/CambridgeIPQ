#include <math.h>
#include <stdlib.h>

#include <glm/glm.hpp>
struct mesh {
  std::vector<glm::vec3> vertices;
  std::vector<glm::vec3> normals;

  std::vector<int> triangles;
  inline mesh(std::vector<glm::vec3> v, std::vector<glm::vec3> n) {
    vertices = v;
    normals = n;
  }
};
std::vector<mesh> scene;
static void addmesh(mesh m) { scene.push_back(m); }
static mesh getMesh(int id) { return scene[id]; }