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
  bool processed; // a new flag introduced for ease to check processed polygons
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

  vertex(vec2 c, float a = 0.0f, bool i = false, vertex *n = nullptr,
         vertex *p = nullptr, vertex *np = nullptr, vertex *nb = nullptr) {
    coords = c;
    next = n;
    prev = p;
    nextPoly = np;
    intersect = i;
    entry = false;
    neighbor = nb;
    alpha = a;
    processed = false;
  }

  std::string toString() {
    // for testing purposes only
    return "[(" + std::to_string(coords.x) + ", " + std::to_string(coords.y) +
           ") intersect " + (intersect ? "true" : "false") + " entry " +
           (entry ? "true" : "false") + " alpha " + std::to_string(alpha) + "]";
  }
};

struct BRay {
  vec3 origin;
  vec3 direction;
  BRay(vec3 o, vec3 d) {
    origin = o;
    direction = d;
  };
  std::string toString() {
    return std::to_string((origin.x)) + " " + std::to_string((origin.y)) + " " +
           std::to_string((origin.z)) + " " + std::to_string((direction.x)) +
           " " + std::to_string((direction.y)) + " " +
           std::to_string((direction.z));
  };
};

float linePlaneIntersection(BRay line, vec3 point, vec3 normal,
                            vec3 *intersection) {
  // return the point where a line and a plane intersect
  // false if the plane is in the back of line direction
  // n*(P_o - P) = 0
  // n*P = n*P_o
  // P = O + tD
  // n*(O + tD) = n*P_o
  // n*O + t*n*D = n*P_o
  // t = (n*P_o - n*O) / n*D
  if (abs(dot(normal, line.direction)) < 0.0001f) {
    return false; // parallel to the plane
  }
  float t = (dot(normal, point) - dot(normal, line.origin)) /
            dot(normal, line.direction);
  *intersection = line.origin + t * line.direction;
  return t;
}

vec2 vector3To2(vec3 u, vec3 v, vec3 origin, vec3 point) {
  vec3 d = point - origin;
  return vec2(dot(u, d) / dot(u, u), dot(v, d) / dot(v, v));
}

vec3 vector2To3(vec3 u, vec3 v, vec3 origin, vec2 point) {
  return origin + u * point.x + v * point.y;
}

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
    if ((p->coords.y > point.y) != (n->coords.y > point.y)) {
      float ratio = (p->coords.y - point.y) / (p->coords.y - n->coords.y);
      if (ratio * (n->coords.x - p->coords.x) + p->coords.x > point.x) {
        count += 1;
      } else {
      }
    } else {
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

void GHphase2(vertex *subject, vertex *clip, bool difference = false) {
  // Going through the edges and set each intersecting
  // points to either entry or exit
  vertex *subjectFirst = subject;
  vertex *clipFirst = clip;
  bool entry = !pointInPolygon(clip, subject->coords);
  entry = entry ^ difference; // different boolean flip the value
  std::cout << "2 pip " << clip->toString() << " (" << subject->coords.x << ", "
            << subject->coords.y << ") " << entry << std::endl;
  do {
    if (subject->intersect) {
      subject->entry = entry;
      entry = !entry;
    }
    subject = subject->next;
  } while (subject != subjectFirst);
  entry = !pointInPolygon(subject, clip->coords);
  std::cout << "2 pip " << subject->toString() << " (" << clip->coords.x << ", "
            << clip->coords.y << ") " << entry << std::endl;
  do {
    if (clip->intersect) {
      clip->entry = entry;
      entry = !entry;
    }
    clip = clip->next;
  } while (clip != clipFirst);
}

vertex *GHphase3(vertex *subject, vertex *clip) {
  // walk through entry and exit points and construct clipped polygons
  vertex *current = subject;
  vertex *startPoly = subject;
  vertex *start = subject;

  vertex *result = nullptr;
  vertex *resultHead = nullptr;
  while (true) {
    // find the intersecting point of subject polygon that isnt processed yet
    while (current->intersect == false || current->processed == true) {
      current = current->next;
      // return if looping back to the start of polygon without finding next
      // intersection point that is unprocessed
      if (startPoly->coords == current->coords) {
        std::cout << "3 ended" << std::endl;
        return result;
      }
    }
    // denote the start of each sub-polygon of the intersection
    // most intersection will only create one polygon but not all
    start = current;
    std::vector<vec2> points;
    do {
      bool isEntry = current->entry;
      do {
        current->processed = true;
        // Move forward at entry points and backward at exit points
        if (isEntry) {
          current = current->next;
        } else {
          current = current->prev;
        }
        points.push_back(current->coords);
        std::cout << "3 push back " << std::to_string(current->coords.x) << " "
                  << std::to_string(current->coords.y) << " " << points.size()
                  << std::endl;
      } while (!current->intersect);

      // move to neighbor polygon after each streak move in one polygon
      current->processed = true;
      current = current->neighbor;
    } while (current->coords != start->coords); // repeat until polygon closed
    // build the polygon and connect to the last polygon
    result = buildPolygon(points);
    for (int i = 0; i < 10; i++) {
      result = result->prev;
      std::cout << "33 result inside " << result << " " << result->toString()
                << " " << (result == nullptr) << std::endl;
    }
    result->nextPoly = resultHead;
    resultHead = result;
  }
}

void polygonClipping(std::vector<BRay> beam, std::vector<vec3> face) {
  // assume each polygons have at least 3 vertices which is non degenerate
  vec3 normal = cross(face[1] - face[0], face[2] - face[0]);
  vec3 origin = face[0];
  std::cout << "poly face 0 " << face[0].x << " " << face[0].y << " "
            << face[0].z << std::endl;
  std::cout << "poly face 1 " << face[1].x << " " << face[1].y << " "
            << face[1].z << std::endl;
  std::cout << "poly face 2 " << face[2].x << " " << face[2].y << " "
            << face[2].z << std::endl;
  vec3 *intersection = new vec3(0, 0, 0);
  std::vector<vec3> subject = {};
  for (int i = 0; i < beam.size(); i++) {
    float in = linePlaneIntersection(beam[i], origin, normal, intersection);
    std::cout << "(" << beam[i].origin.x << " " << beam[i].origin.y << " "
              << beam[i].origin.z << ") (" << beam[i].direction.x << " "
              << beam[i].direction.y << " " << beam[i].direction.z << ") ";
    std::cout << intersection->x << " " << intersection->y << " "
              << intersection->z << " in? " << in << std::endl;
    if (in > 0) {
      subject.push_back(*intersection);
    }
  }
  if (subject.size() < 4) {
    return;
  }

  // u point rightward and v upward, where normal point toward camera
  vec3 v = face[1] - face[0];
  vec3 u = cross(normal, v);
  std::vector<vec2> subject2D = {};
  std::vector<vec2> clip2D = {};
  vertex *subjectList;
  vertex *clipList;
  std::cout << "poly 3to2 normal " << normal.x << " " << normal.y << " "
            << normal.z << std::endl;
  std::cout << "poly 3to2 u " << u.x << " " << u.y << " " << u.z << std::endl;
  std::cout << "poly 3to2 v " << v.x << " " << v.y << " " << v.z << std::endl;
  std::cout << "poly 3to2 origin " << origin.x << " " << origin.y << " "
            << origin.z << std::endl;
  for (int i = 0; i < subject.size(); i++) {
    subject2D.push_back(vector3To2(u, v, origin, subject[i]));
  }
  for (int i = 0; i < face.size(); i++) {
    clip2D.push_back(vector3To2(u, v, origin, face[i]));
  }
  // std::cout << "poly done to 2d" << std::endl;
  subjectList = buildPolygon(subject2D);
  clipList = buildPolygon(clip2D);
  for (int i = 0; i < 10; i++) {
    std::cout << "poly 0 subject " << subjectList << " "
              << subjectList->toString() << std::endl;
    subjectList = subjectList->next;
  }
  for (int i = 0; i < 10; i++) {
    std::cout << "poly 0 clip " << clipList << " " << clipList->toString()
              << std::endl;
    clipList = clipList->next;
  }
  GHphase1(subjectList, clipList);
  GHphase2(subjectList, clipList);
  vertex *result = GHphase3(subjectList, clipList);
  vertex *resultHead = result;
  if (result == nullptr) {
    if (pointInPolygon(subjectList, clipList->coords)) {
      std::cout << "poly no intersection clip inside" << std::endl;
    } else if (pointInPolygon(clipList, subjectList->coords)) {
      std::cout << "poly no intersection clip outside" << std::endl;
    } else {
      std::cout << "poly no intersection disjoint" << std::endl;
    }
    return;
  }
  for (int i = 0; i < 10; i++) {
    vec3 temp = vector2To3(u, v, origin, result->coords);
    std::cout << "poly result " << temp.x << " " << temp.y << " " << temp.z
              << std::endl;
    ;
    result = result->next;
    if (result == resultHead) {
      if (result->nextPoly != nullptr) {
        result = result->nextPoly;
        resultHead = result;
        std::cout << std::endl;
      } else {
        break;
      }
    }
  }
}

std::vector<vec2> testClipping(std::vector<vec2> subject,
                               std::vector<vec2> clip) {
  std::cout << "test 00 " << subject.size() << " " << clip.size() << std::endl;
  vertex *subjectList = buildPolygon(subject);
  vertex *clipList = buildPolygon(clip);
  std::cout << "test 0" << std::endl;
  GHphase1(subjectList, clipList);
  std::cout << "test 1" << std::endl;
  GHphase2(subjectList, clipList, true);
  std::cout << "test 2" << std::endl;
  vertex *result = GHphase3(subjectList, clipList);
  std::cout << "test 3" << std::endl;
  vertex *resultHead = result;
  if (result == nullptr) {
    return {};
  }
  std::vector<vec2> resultCoords = {};
  for (int i = 0; i < 10; i++) {
    std::cout << "poly result " << result->coords.x << " " << result->coords.y
              << std::endl;
    resultCoords.push_back(result->coords);
    result = result->next;
    if (result == resultHead) {
      if (result->nextPoly != nullptr) {
        result = result->nextPoly;
        resultCoords.push_back(vec2(INFINITY, INFINITY));
        resultHead = result;
        std::cout << std::endl;
      } else {
        break;
      }
    }
  }
  resultCoords.push_back(vec2(INFINITY, INFINITY));
  return resultCoords;
}

int main2() {
  std::vector<BRay> beam;
  beam.push_back(BRay(vec3(0, 0, 1), vec3(-1, 1, 1)));
  beam.push_back(BRay(vec3(0, 0, 1), vec3(1, 1, 1)));
  beam.push_back(BRay(vec3(0, 0, 1), vec3(1, 0, 1)));
  beam.push_back(BRay(vec3(0, 0, 1), vec3(-1, 0, 1)));

  std::vector<vec3> tempv = {
      vec3(-1, 1, 1) + vec3(0, 0, 5),   vec3(1, 1, 1) + vec3(0, 0, 5),
      vec3(-1, 1, -1) + vec3(0, 0, 5),  vec3(1, 1, -1) + vec3(0, 0, 5),
      vec3(-1, -1, 1) + vec3(0, 0, 5),  vec3(1, -1, 1) + vec3(0, 0, 5),
      vec3(-1, -1, -1) + vec3(0, 0, 5), vec3(1, -1, -1) + vec3(0, 0, 5)};
  int tempi[36] = {0, 1, 2, 1, 3, 2, 6, 5, 4, 6, 7, 5, 0, 5, 1, 0, 4, 5,
                   2, 3, 6, 3, 7, 6, 0, 2, 4, 2, 6, 4, 1, 5, 3, 3, 5, 7};
  for (int i = 0; i < 12; i++) {
    std::cout << "polygon clipping" << std::endl;
    // polygonClipping(beam, std::vector<vec3>{tempv[2], tempv[3], tempv[6]});
    polygonClipping(beam, std::vector<vec3>{tempv[tempi[3 * i]],
                                            tempv[tempi[3 * i + 1]],
                                            tempv[tempi[3 * i + 2]]});
  }
  return 0;
  std::vector<vec2> subjectData = {vec2(1, 3), vec2(1, 7), vec2(7, 7),
                                   vec2(7, 3)};
  std::vector<vec2> clipData = {vec2(2, 1), vec2(2, 5), vec2(4, 2),
                                vec2(6.5f, 4.5f), vec2(6.5f, 1.5f)};
  // std::reverse(clipData.begin(), clipData.end());
  vertex *subject = buildPolygon(subjectData);
  vertex *clip = buildPolygon(clipData);
  vertex *subjectHead = subject;
  vertex *clipHead = clip;
  std::cout << "First subject " << subject << " " << subject->toString()
            << std::endl;

  std::cout << "In polygon check " << pointInPolygon(clip, vec2(3, 3))
            << std::endl;

  for (int i = 0; i < 10; i++) {
    std::cout << "0 subject " << subject << " " << subject->toString()
              << std::endl;
    subject = subject->next;
  }
  for (int i = 0; i < 10; i++) {
    std::cout << "0 clip " << clip << " " << clip->toString() << std::endl;
    clip = clip->next;
  }

  GHphase1(subject, clip);

  for (int i = 0; i < 10; i++) {
    std::cout << "1 subject " << subject << " " << subject->toString()
              << std::endl;
    subject = subject->next;
  }
  for (int i = 0; i < 10; i++) {
    std::cout << "1 clip " << clip << " " << clip->toString() << std::endl;
    clip = clip->next;
  }
  subject = subjectHead;
  clip = clipHead;
  GHphase2(subject, clip);
  for (int i = 0; i < 10; i++) {
    std::cout << "2 subject " << subject << " " << subject->toString() << " "
              << subject->entry << std::endl;
    subject = subject->next;
  }
  for (int i = 0; i < 10; i++) {
    std::cout << "2 clip " << clip << " " << clip->toString() << " "
              << clip->entry << std::endl;
    clip = clip->next;
  }
  vertex *result = GHphase3(subject, clip);
  vertex *resultHead = result;
  for (int i = 0; i < 10; i++) {

    std::cout << "3 result " << result << " " << result->toString() << " "
              << (result == nullptr) << std::endl;
    result = result->next;
    if (result == resultHead) {
      if (result->nextPoly != nullptr) {
        result = result->nextPoly;
        resultHead = result;
        std::cout << std::endl;
      } else {
        break;
      }
    }
  }
  return 0;
}
