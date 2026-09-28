#pragma once
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <glm/common.hpp>
#include <glm/ext/matrix_float3x3.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_int2.hpp>
#include <glm/geometric.hpp>
#include <stdlib.h>

#include <glm/glm.hpp>
#include <iostream>
#include <sys/types.h>
#include <vector>

#include "camera.h"
#include "color.h"
#include "mesh.h"
#include "renderer.h"
void computeCameraMatrixes();
void traceRays();