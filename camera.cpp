#include <glm/glm.hpp>
using namespace glm;

vec3 cameraPosition = vec3(0.0f, 0.0f, 0.0f);
vec3 cameraRotation = vec3(-0.5f, 0.0f, 0.0f);

ivec2 getScreenResolution() { return ivec2(640, 360); }

void setCameraPosition(vec3 pos) { cameraPosition = pos; }
vec3 getCameraPosition() { return cameraPosition; }

void setCameraRotation(vec3 pos) { cameraRotation = pos; }
vec3 getCameraRotation() { return cameraRotation; }
