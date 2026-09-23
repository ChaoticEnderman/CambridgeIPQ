#pragma once
#include <glm/glm.hpp>

glm::ivec2 getScreenResolution();
constexpr int getScreenResolutionX() { return 640; }
constexpr int getScreenResolutionY() { return 640; }

void setCameraPosition(glm::vec3 pos);
glm::vec3 getCameraPosition();

void setCameraRotation(glm::vec3 pos);
glm::vec3 getCameraRotation();