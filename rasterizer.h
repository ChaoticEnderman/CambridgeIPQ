#include <stdlib.h>

#include <glm/glm.hpp>

#include "camera.h"
#include "mesh.h"
#include "renderer.h"

void raster2DLine(glm::ivec2 v1, glm::ivec2 v2);
void raster3DLine(glm::vec3 v1, glm::vec3 v2);

void raster3Dcoord();

void raster2DTriangle(glm::ivec2 vertices[], color c);
void raster3DTriangle(glm::vec3 vertices[], color c);
void raster3DMesh(mesh m);

void computeRotationMatrixes();

void rasterPoint(glm::vec3 vertices);