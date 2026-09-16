#include <glm/glm.hpp>

glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraRotation = glm::vec3(0.0f, 0.0f, 0.0f);
const glm::ivec2 screenResolution = glm::ivec2(640, 640);

glm::ivec2 getScreenResolution() { return screenResolution; }

void setCameraPosition(glm::vec3 pos) { cameraPosition = pos; }
glm::vec3 getCameraPosition() { return cameraPosition; }

void setCameraRotation(glm::vec3 pos) { cameraRotation = pos; }
glm::vec3 getCameraRotation() { return cameraRotation; }
