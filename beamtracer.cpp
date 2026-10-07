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

  vertex(vec2 c, float alpha = 0.0f, bool i = false, vertex *n = nullptr,
         vertex *p = nullptr, vertex *np = nullptr, vertex *nb = nullptr) {
    coords = c;
    next = n;
    prev = p;
    nextPoly = np;
    intersect = i;
    entry = false;
    neighbor = nb;
    alpha = 0.0f;
  }

  std::string toString() {
    // for testing purposes only
    return "[(" + std::to_string(coords.x) + ", " + std::to_string(coords.y) +
           ") " + "]";
  }
};

// helper functionalities to build a linked list polygon from normal array of
// vertices
vertex *buildPolygon(std::vector<vec2> vertices) {
  vertex *head = new vertex(vertices[0]);
  vertex *current = head;

  for (int i = 1; i < vertices.size(); i++) {
    vertex *vert = new vertex(vertices[i]);
    current->next = vert;
    vert->prev = current;
    current = current->next;
  }
  current->next = head;
  head->prev = current;
  return head;
}

bool pointInPolygon(vertex *p, vec2 point) {
  vertex *first = p;
  vertex *n = p->next;
  int count = 0;
  do {
    // one point is on the ray and one point is under it
    std::cout << "Polygon checking " << p->toString();
    if ((p->coords.y > point.y) != (n->coords.y > point.y)) {
      float ratio = (p->coords.y - point.y) / (p->coords.y - n->coords.y);
      if (ratio * (n->coords.x - p->coords.x) + p->coords.x > point.x) {
        count += 1;
        std::cout << "+1";
      } else {
        std::cout << "+0";
      }
    } else {
      std::cout << "+0";
    }
    p = n;
    n = n->next;
    std::cout << std::endl;
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

void GHphase2(vertex *subject, vertex *clip) {
  // Going through the edges and set each intersecting
  // points to either entry or exit
  vertex *subjectFirst = subject;
  vertex *clipFirst = clip;
  bool entry = pointInPolygon(subject, clip->coords);
  do {
    if (subject->intersect) {
      subject->entry = entry;
      entry = !entry;
    }
    subject = subject->next;
  } while (subject != subjectFirst);
  entry = pointInPolygon(clip, clip->coords);
  do {
    if (clip->intersect) {
      clip->entry = entry;
      entry = !entry;
    }
    clip = clip->next;
  } while (clip != clipFirst);
}

void GHphase3(vertex *subject, vertex *clip) {
  vertex *subjectFirst = subject;
  vertex *clipFirst = clip;
  while (!subject->intersect) {
    subject = subject->next;
  }
  vertex *result = new vertex(subject->coords, 0.0f, false);
  while (true) {
    if (subject->entry) {
      do {
        // Moving forward until next intersection point to build polygon
        subject = subject->next;
        vertex *add = new vertex(subject->coords);
        result->next = add;
        add->prev = result;
        result = result->next;
      } while (subject->intersect);
    } else {
      do {
        subject = subject->prev;
        // Moving backward until next intersection point to build polygon
        vertex *add = new vertex(subject->coords);
        result->next = add;
        add->prev = result;
        result = result->next;
      } while (subject->intersect);
    }
    subject = subject->neighbor;
  }
}

int main() {
  std::vector<vec2> subjectData = {vec2(1, 3), vec2(1, 7), vec2(7, 7),
                                   vec2(7, 3)};
  std::vector<vec2> clipData = {vec2(2, 1), vec2(2, 5), vec2(4, 2),
                                vec2(6.5f, 4.5f), vec2(6.5f, 1.5f)};

  vertex *subject = buildPolygon(subjectData);
  vertex *clip = buildPolygon(clipData);
  vertex *subjectHead = subject;
  vertex *clipHead = clip;
  std::cout << "First subject " << subject << " " << subject->toString()
            << std::endl;

  std::cout << "In polygon check " << pointInPolygon(clip, vec2(1, 3))
            << std::endl;

  // for (int i = 0; i < 10; i++) {
  //   std::cout << "Subject " << subject << " " << subject->toString()
  //             << std::endl;
  //   subject = subject->prev;
  // }
  // for (int i = 0; i < 10; i++) {
  //   std::cout << "Clip " << clip << " " << clip->toString()
  //             << std::endl;
  //   clip = clip->prev;
  // }

  GHphase1(subject, clip);
  // for (int i = 0; i < 10; i++) {
  //   if (subject->intersect) {
  //     std::cout << "Subject " << subject << " " << subject->toString()
  //               << std::endl;
  //   }
  //   subject = subject->prev;
  // }
  // for (int i = 0; i < 10; i++) {
  //   if (clip->intersect) {
  //     std::cout << "Clip " << clip << " " << clip->toString() << std::endl;
  //   }
  //   clip = clip->prev;
  // }
  subject = subjectHead;
  clip = clipHead;
  GHphase2(subject, clip);
  for (int i = 0; i < 10; i++) {
    if (subject->intersect) {
      std::cout << "Subject " << subject << " " << subject->toString() << " "
                << subject->entry << std::endl;
    }
    subject = subject->prev;
  }
  for (int i = 0; i < 10; i++) {
    if (clip->intersect) {
      std::cout << "Clip " << clip << " " << clip->toString() << " "
                << subject->entry << std::endl;
    }
    clip = clip->prev;
  }
  return 0;
}
