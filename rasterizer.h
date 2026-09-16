#include <stdlib.h>

#include <glm/glm.hpp>

#include "camera.h"
#include "renderer.h"

void raster2DLine(glm::ivec2 v1, glm::ivec2 v2);

void raster2DTriangle(glm::ivec2 vertices[], int r, int g, int b);
void raster3DTriangle(glm::vec3 vertices[], int r, int g, int b);
void raster3DMesh(glm::vec3 vertices[], int indices[], int count);

void computeRotationMatrixes();

void rasterPoint(glm::vec3 vertices);