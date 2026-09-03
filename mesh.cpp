#include <math.h>
#include <stdlib.h>

#include <glm/glm.hpp>

namespace Mesh {
std::vector<mesh> scene;

struct mesh {
  std::vector<glm::vec3> vertices;
  std::vector<glm::vec3> normals;

  std::vector<int> triangles;
  inline mesh(std::vector<glm::vec3> v, std::vector<glm::vec3> n) {
    vertices = v;
    normals = n;
  }
};
static mesh addmesh(mesh m) { scene.push_back(m); }
}  // namespace Mesh