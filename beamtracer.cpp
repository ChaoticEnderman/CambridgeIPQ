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

bool segmentIntersect(vec2 a, vec2 b, vec2 c, vec2 d, float *t, float *u,
                      vec2 *point) {
  // represent line ab as P + tR and cd as Q + uS
  vec2 p = a;
  vec2 r = b - a;
  vec2 q = c;
  vec2 s = d - c;
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
  *t = cross2D(q - p, s) * rs;
  *u = cross2D(q - p, r) * rs;
  *point = p + (*t * r);
  // check if the intersection is inside the segment or not
  return *t > 0.0f && *t < 1.0f && *u > 0.0f && *u < 1.0f;
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
  void insert(vertex *s1, vertex *s2, vertex *data) {
    vertex *c = s1->next;
    while (c != s2 && c->alpha < data->alpha) {
      c = c->next;
    }

    // after the loop we get to the node after c
    // so this step will insert before c
    data->next = c;
    data->prev = c->prev;
    c->prev->next = data;
    c->prev = data;
  }

  vertex(vec2 c, float alpha, bool i, vertex *n = nullptr, vertex *p = nullptr,
         vertex *np = nullptr, vertex *nb = nullptr) {
    coords = c;
    next = n;
    prev = p;
    nextPoly = np;
    intersect = i;
    entry = false;
    neighbor = nb;
    alpha = 0.0f;
  }
};

bool pointInPolygon(vertex *p, vec2 point) {
  vertex *first = p;
  vertex *n = p->next;
  int count = 0;
  do {
    // one point is on the ray and one point is under it
    if (sign(p->coords.x - point.x) != sign(n->coords.x - point.x)) {
      float ratio = (p->coords.y - point.y) / (p->coords.y - n->coords.y);
      if (ratio * (n->coords.x - p->coords.x) + p->coords.x > 0.0f) {
        count += 1;
      }
    }
    p = n;
    n = n->next;
  } while (p != first);
  return count % 2;
}

void GHphase1(vertex *subject, vertex *clip) {
  vertex *subjectFirst = subject, *clipFirst = clip;
  vertex *subjectNext = subject, *clipNext = clip;
  float t, u;
  vec2 point;
  do {
    subjectNext = subjectNext->next;
    while (subjectNext->intersect == true) {
      // continue passing all intersect points until it reach polygon vertex
      subjectNext = subjectNext->next;
    }
    do {
      clipNext = clipNext->next;
      while (clipNext->intersect == true) {
        // continue passing all intersect points until it reach polygon vertex
        clipNext = clipNext->next;
      }

      if (segmentIntersect(subject->coords, subjectNext->coords, clip->coords,
                           clipNext->coords, &t, &u, &point)) {
        vertex *i1 = new vertex(point, t, true);
        vertex *i2 = new vertex(point, u, true);
        // insert to appropriate position between the vertices
        (*i1).insert(subject, subjectNext, i1);
        (*i2).insert(clip, clipNext, i2);
        // linking neighbor
        i1->neighbor = i2;
        i2->neighbor = i1;
      }
      clip = clipNext;
    } while (clip != clipFirst);
    subject = subjectNext;
  } while (subject != subjectFirst);
}
