#pragma once
#include <stdlib.h>

#include <glm/glm.hpp>

#include "camera.h"
#include "mesh.h"
#include "renderer.h"

using namespace glm;

void raster2DLine(ivec2 v1, ivec2 v2);
void raster3DLine(vec3 v1, vec3 v2);

void raster3Dcoord();

void raster2DTriangle(ivec2 vertices[], color c);
void raster3DTriangle(vec3 vertices[], color c);
void raster3DMesh(mesh m);

void clearZBuffer();
void computeRotationMatrixes();

void rasterPoint(vec3 vertices);