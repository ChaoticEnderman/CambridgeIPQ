#pragma once
#include <SDL2/SDL_render.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <glm/common.hpp>
#include <glm/ext/matrix_float3x3.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_int2.hpp>
#include <glm/geometric.hpp>
#include <iterator>
#include <ostream>
#include <stdlib.h>

#include <glm/glm.hpp>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <vector>

#include "camera.h"
#include "color.h"
#include "mesh.h"
#include "renderer.h"

#include <stdlib.h>
#include <vector>

std::vector<vec2> testClipping(std::vector<vec3> subject,
                               std::vector<vec3> clip);

void traceBeams(vec3 apex = vec3(0, 0, 0), vec3 d = vec3(0, 0, 1),
                std::vector<vec2> polygon = {vec2(-1, -1), vec2(-1, 1),
                                             vec2(1, 1), vec2(1, -1)});
