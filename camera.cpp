#include <glm/glm.hpp>

class camera {
  glm::vec3 cameraPosition;
  glm::vec3 cameraRotation;

  void setCameraPosition(glm::vec3 pos) { cameraPosition = pos; }
  glm::vec3 getCameraPosition() { return cameraPosition; }

  void setCameraRotation(glm::vec3 pos) { cameraRotation = pos; }
  glm::vec3 getCameraRotation() { return cameraRotation; }
};  // namespace camera
