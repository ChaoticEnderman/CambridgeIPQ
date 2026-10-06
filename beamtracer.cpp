#include <SDL2/SDL_render.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <glm/common.hpp>
#include <glm/ext/matrix_float3x3.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_int2.hpp>
#include <glm/geometric.hpp>
#include <iterator>
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

using namespace glm;

const float epsilon = 0.0001;

inline float cross2D(vec2 ab, vec2 ac) { return ab.x * ac.y - ab.y * ac.x; }

static inline int sgn(int64_t v) { return (v > 0) - (v < 0); }

bool segmentIntersect(vec2 a, vec2 b, vec2 c, vec2 d, vec2 *point) {
  // represent line ab as P + tR and cd as Q + uS
  vec2 p = a;
  vec2 r = b - a;
  vec2 q = c;
  vec2 s = d - c;
  //   vec2 t = (q - p) * s / cross2D(r, s);
  //   vec2 u = (q - p) * r / cross2D(r, s);
  // optimizing repetive code r*s
  float rs = cross2D(r, s);
  if (abs(rs) < epsilon) {
    // early bail out since rs = 0 mean the segments are either colinear or
    // parallel which we consider to be non-intersecting for algorithm purposes
    // as well as preventing division by zero
    return false;
  }
  rs = 1 / rs;
  // P + tR and Q + uS are hypothetical infinite lines on the direction of each
  // segment
  float t = cross2D(q - p, s) * rs;
  float u = cross2D(q - p, r) * rs;
  *point = p + (t * r);
  // check if the intersection is inside the segment or not
  return t > 0.0f && t < 1.0f && u > 0.0f && u < 1.0f;
}

struct vertex {
  // polygon vertex struct from Efficient Clipping of Arbitrary Polygons paper
  // https://davis.wpi.edu/~matt/courses/clipping/
  // https://dl.acm.org/doi/epdf/10.1145/274363.274364
  vec2 coords;
  vertex *next;
  vertex *prev;
  vertex *nextPoly;
  bool intersect;
  bool entry;
  vertex *neighbor;
  float alpha;
  void insert(vertex *next, vertex data) {
    data.next = next;
    next = &data;
  }
  vertex(vec2 c, vertex *n = nullptr, vertex *p = nullptr, vertex *np = nullptr,
         vertex *nb = nullptr) {
    coords = c;
    next = n;
    prev = p;
    nextPoly = np;
    intersect = false;
    entry = false;
    neighbor = nb;
    alpha = 0.0f;
  }
};

void test(vertex *subject, vertex *clip) {
  vertex *subjectFirst = subject, *clipFirst = clip;
  vec2 point;
  do {
    do {

      if (segmentIntersect(subject->coords, subject->next->coords, clip->coords,
                           clip->next->coords, &point)) {
        vertex i1 = vertex(point);

        clip = clip->next;
      }
    } while (clip != clipFirst);
    subject = subject->next;
  } while (subject != subjectFirst);
}
