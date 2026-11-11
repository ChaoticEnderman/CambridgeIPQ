#include "beamtracer.h"
#include <SDL2/SDL_render.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <glm/common.hpp>
#include <glm/detail/qualifier.hpp>
#include <glm/ext/matrix_float3x3.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_int2.hpp>
#include <glm/geometric.hpp>
#include <ios>
#include <iterator>
#include <ostream>
#include <ratio>
#include <stdlib.h>

#include <algorithm>
#include <glm/glm.hpp>
#include <iostream>
#include <numeric>
#include <string>
#include <sys/types.h>
#include <tuple>
#include <vector>

#include "camera.h"
#include "color.h"
#include "mesh.h"
#include "rasterizer.h"
#include "renderer.h"

using namespace glm;
using namespace std;

const float EPSILON = 0.0001f;
const int MAXDEPTH = 1;
// float camLen = 0.1f;

inline float cross2D(vec2 ab, vec2 ac) { return ab.x * ac.y - ab.y * ac.x; }

static inline int sgn(int64_t v) { return (v > 0) - (v < 0); }

// mat4x4 cameraMat = mat4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

string printVec(vec3 v) {
  return "(" + to_string(v.x) + " " + to_string(v.y) + " " + to_string(v.z) +
         ")";
}

// bool segmentIntersect(vec2 a, vec2 b, vec2 *c, vec2 *d, float *t, float *u,
//                       vec2 *point) {
//   // represent line ab as P + tR and cd as Q + uS
//   vec2 p = a;
//   vec2 r = b - a;
//   float len = length(r);
//   if (len < EPSILON) {
//     // this can cause division by error
//     return false;
//   }
//   vec2 n = vec2(-r.y, r.x) / len;
//   // distance of c and d to line ab
//   float dc = dot(*c - a, n);
//   float dd = dot(*d - a, n);
//   if (abs(dc) < EPSILON) {
//     // perturb c in direction of n
//     *c += n * (EPSILON * 2 - dc);
//   }
//   if (abs(dd) < EPSILON) {
//     // perturb d in direction of n
//     *d += n * (EPSILON * 2 - dd);
//   }
//   // calculate q and s after perturbing
//   vec2 q = *c;
//   vec2 s = *d - *c;
//   float rs = cross2D(r, s);

//   if (abs(rs) < EPSILON) {
//     // early bail out since rs = 0 mean the segments are either colinear or
//     // parallel which we consider to be non-intersecting for algorithm
//     purposes
//     // as well as preventing division by zero
//     return false;
//   }
//   rs = 1 / rs;
//   // P + tR and Q + uS are hypothetical infinite lines on the direction of
//   each
//   // segment
//   *t = cross2D(q - p, s) * rs;
//   *u = cross2D(q - p, r) * rs;
//   *point = p + (*t * r);
//   // check if the intersection is inside the segment or not
//   return *t >= 0.0f && *t < 1.0f && *u >= 0.0f && *u < 1.0f;
// }

bool segmentIntersect(vec2 a, vec2 b, vec2 c, vec2 d, float *t, float *u,
                      bool *colinear, vec2 *point) {
  vec2 r = b - a, s = d - c;

  // distances
  float dc = cross2D(r, c - a);
  float dd = cross2D(r, d - a);
  float da = cross2D(s, a - c);
  float db = cross2D(s, b - c);

  *colinear = cross2D(r, s) == 0;

  // return when they are on the same side
  if ((dc >= 0.0f) == (dd >= 0.0f)) {
    return false;
  }
  if ((da >= 0.0f) == (db >= 0.0f)) {
    return false;
  }

  *t = da / (da - db);
  *u = dc / (dc - dd);
  *point = a + *t * r;
  return true;
}

enum INTERSECT {
  NONE,
  CROSSING,
  BOUNCING,
  LEFT_ON,
  RIGHT_ON,
  ON_ON,
  ON_LEFT,
  ON_RIGHT,
  DELAYED_CROSSING,
  DELAYED_BOUNCING
};

enum TURN {
  LEFT,
  RIGHT,
  IS_PP,
  IS_PM,
};

enum ENTRYEXIT { // for marking intersection vertices as "entry" or "exit"
  EXIT,
  ENTRY,
  NEITHER
};

struct vertex {
  // polygon vertex struct from Efficient Clipping of Arbitrary Polygons paper
  // https://davis.wpi.edu/~matt/courses/clipping/
  // https://dl.acm.org/doi/epdf/10.1145/274363.274364
  vec2 coords;
  vertex *next;
  vertex *prev;
  vertex *nextPoly;
  bool intersect;
  INTERSECT label;
  ENTRYEXIT entry;
  vertex *neighbor;
  float alpha;
  bool processed; // a new flag introduced for ease to check processed polygons
  bool inserted;  // a new flag for foster et al improvement phase 1
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
         vertex *p = nullptr, vertex *np = nullptr, vertex *nb = nullptr,
         bool ins = false) {
    coords = c;
    next = n;
    prev = p;
    nextPoly = np;
    intersect = i;
    label = NONE;
    neighbor = nb;
    alpha = a;
    processed = false;
    inserted = true;
  }

  string toString() {
    // for testing purposes only
    return "[(" + to_string(coords.x) + ", " + to_string(coords.y) +
           ") intersect " + (intersect ? "true" : "false") + " entry " +
           to_string(label) + " alpha " + to_string(alpha) + "]";
  }
};

// float linePlaneIntersection(BRay line, vec3 point, vec3 normal,
//                             vec3 *intersection) {
//   // return the point where a line and a plane intersect
//   // false if the plane is in the back of line direction
//   // n*(P_o - P) = 0
//   // n*P = n*P_o
//   // P = O + tD
//   // n*(O + tD) = n*P_o
//   // n*O + t*n*D = n*P_o
//   // t = (n*P_o - n*O) / n*D
//   if (abs(dot(normal, line.direction)) < 0.0001f) {
//     return false; // parallel to the plane
//   }
//   float t = (dot(normal, point) - dot(normal, line.origin)) /
//             dot(normal, line.direction);
//   *intersection = line.origin + t * line.direction;
//   return t;
// }

vec2 vector3To2(vec3 u, vec3 v, vec3 origin, vec3 point) {
  vec3 d = point - origin;
  return vec2(dot(u, d) / dot(u, u), dot(v, d) / dot(v, v));
}

vec3 vector2To3(vec3 u, vec3 v, vec3 origin, vec2 point) {
  return origin + u * point.x + v * point.y;
}

vec3 findVirtualApex(vec3 point, vec3 normal, vec3 apex) {
  // point P, apex is point V, normal vector n
  // let point M lies on the plane, MV is perpendicular to plane
  // point p and point m lies on the plane
  // (V-M).n = 0 and (V-P).n = 0 and (M-P).n = 0 (1)
  // we can see V = M - t.n with some constant t (2)
  // M = V + t.n
  // substitute (2) to (1): (V + t.n - P).n = 0
  // V.n + t.n.n - P.n = 0
  // t = n(V-P) / n.n
  // flipping V through the plane give virtual apex V'
  // V' = M + t.n = V + 2.t.n
  float t = (dot(normal, (apex - point)) / (dot(normal, normal)));
  return apex + 2 * t * normal;
}

// helper functionalities to build a linked list polygon from normal array of
// vertices
vertex *buildPolygon(vector<vec2> vertices) {
  vertex *head = new vertex(vertices[0]);
  head->inserted = false;
  vertex *current = head;

  for (int i = 1; i < vertices.size(); i++) {
    vertex *vert = new vertex(vertices[i]);
    vert->inserted = false;
    current->next = vert;
    vert->prev = current;
    current = current->next;
  }
  current->next = head;
  head->prev = current;
  return head;
}

bool pointInPolygon(vertex *p, vec2 point) {
  // Ray casting to find point in polygon
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

bool polygonClockwise(vertex *polygon) {
  float area = 0.0f;
  vertex *head = polygon;
  do {
    area += polygon->coords.x * polygon->next->coords.y -
            polygon->next->coords.x * polygon->coords.y;
    polygon = polygon->next;
  } while (polygon != head);
  return area < 0.0f;
}

// split a ring shape into two concave polygons
std::tuple<vector<vertex *>, vector<vertex *>>
splitAnnulus(vertex *outside, vertex *inside, const bool outerWindingCw,
             const bool innerWindingCw) {
  vertex *testOutsideHead = outside, *testInsideHead = inside;
  string outMsg = "Annulus outside polygon(";
  for (int i = 0; i < 10; i++) {
    outMsg += "(" + to_string(outside->coords.x) + "," +
              to_string(outside->coords.y) + "),";
    outside = outside->next;
  }
  outMsg.pop_back();
  outMsg += ")";
  cout << outMsg << endl;
  string inMsg = "Annulus inside polygon(";
  for (int i = 0; i < 10; i++) {
    inMsg += "(" + to_string(inside->coords.x) + "," +
             to_string(inside->coords.y) + "),";
    inside = inside->next;
  }
  inMsg.pop_back();
  inMsg += ")";
  cout << inMsg << endl;
  vec2 leftmost(INFINITY, 0), rightmost(-INFINITY, 0);
  vertex *innerLeftEdge, *innerRightEdge;
  vertex *insideHead = inside;
  vertex *outsideHead = outside;
  float ratio;
  float xcoord;
  // first vertex of the face
  vertex *leftEdge;
  vertex *rightEdge;
  do {
    // find leftmost and rightmost vertices of inside polygon
    if (leftmost.x > inside->coords.x) {
      leftmost = inside->coords;
      innerLeftEdge = inside;
    }
    if (rightmost.x < inside->coords.x) {
      innerRightEdge = inside;
      rightmost = inside->coords;
    }
    inside = inside->next;
  } while (inside != insideHead);
  vec2 nearestLeft = vec2(-INFINITY, leftmost.y);
  vec2 nearestRight = vec2(INFINITY, rightmost.y);
  do {
    // raycast to each vertices to find intersection
    if ((outside->coords.y > leftmost.y) !=
        (outside->next->coords.y > leftmost.y)) {
      ratio = (outside->coords.y - leftmost.y) /
              (outside->coords.y - outside->next->coords.y);
      xcoord = (outside->next->coords.x - outside->coords.x) * ratio +
               outside->coords.x;
      if (xcoord < leftmost.x && xcoord > nearestLeft.x) {
        nearestLeft.x = xcoord;
        leftEdge = outside;
      }
    }
    if ((outside->coords.y > rightmost.y) !=
        (outside->next->coords.y > rightmost.y)) {
      ratio = (outside->coords.y - rightmost.y) /
              (outside->coords.y - outside->next->coords.y);
      xcoord = (outside->next->coords.x - outside->coords.x) * ratio +
               outside->coords.x;
      if (xcoord > rightmost.x && xcoord < nearestRight.x) {
        nearestRight.x = xcoord;
        rightEdge = outside;
      }
    }
    outside = outside->next;
  } while (outside != outsideHead);
  std::vector<vec2> topCoords = {};
  std::vector<vec2> bottomCoords = {};

  // insert data in between intersecting edges
  vertex *insertLeft = new vertex(vec2(nearestLeft));
  leftEdge->next->prev = insertLeft;
  insertLeft->next = leftEdge->next;
  insertLeft->prev = leftEdge;
  leftEdge->next = insertLeft;
  vertex *insertRight = new vertex(vec2(nearestRight));
  rightEdge->next->prev = insertRight;
  insertRight->next = rightEdge->next;
  insertRight->prev = rightEdge;
  rightEdge->next = insertRight;

  vertex *current;
  // top outer
  current = insertRight;
  while (current != insertLeft) {
    topCoords.push_back(current->coords);
    current = outerWindingCw ? current->prev : current->next;
  }
  topCoords.push_back(current->coords);
  // top inner
  current = innerLeftEdge;
  while (current != innerRightEdge) {
    topCoords.push_back(current->coords);
    current = innerWindingCw ? current->next : current->prev;
  }
  topCoords.push_back(current->coords);
  // bottom outer
  current = insertLeft;
  while (current != insertRight) {
    bottomCoords.push_back(current->coords);
    current = outerWindingCw ? current->prev : current->next;
  }
  bottomCoords.push_back(current->coords);
  // bottom inner
  current = innerRightEdge;
  while (current != innerLeftEdge) {
    bottomCoords.push_back(current->coords);
    current = innerWindingCw ? current->next : current->prev;
  }
  bottomCoords.push_back(current->coords);
  cout << "TSA size " << topCoords.size() << " " << bottomCoords.size() << endl;
  // for (int i = 0; i < topCoords.size(); i++) {
  //   std::cout << "TSA top vector " << topCoords[i].x << " " << topCoords[i].y
  //             << endl;
  // }
  // for (int i = 0; i < bottomCoords.size(); i++) {
  //   std::cout << "TSA bottom vector " << bottomCoords[i].x << " "
  //             << bottomCoords[i].y << endl;
  // }
  cout << endl;
  vertex *resultTop, *resultBottom;
  resultTop = buildPolygon(topCoords);
  resultBottom = buildPolygon(bottomCoords);
  // for (int i = 0; i < 10; i++) {
  //   std::cout << "TSA top list " << resultTop->coords.x << " "
  //             << resultTop->coords.y << endl;
  //   resultTop = resultTop->next;
  // }
  // for (int i = 0; i < 10; i++) {
  //   std::cout << "TSA bottom list " << resultBottom->coords.x << " "
  //             << resultBottom->coords.y << endl;
  //   resultBottom = resultBottom->next;
  // }
  return {{resultTop, resultBottom}, {inside}};
}

// void testSplitAnnulus() {
//   std::vector<vec2> outside = {vec2(0, 0), vec2(0, 10), vec2(10, 10),
//                                vec2(10, 0)};
//   std::vector<vec2> inside = {vec2(2, 8), vec2(8, 5), vec2(6, 1), vec2(4,
//   1)}; vertex *outsideList = buildPolygon(outside); vertex *insideList =
//   buildPolygon(inside); vertex *top, *bottom; std::tie(top, bottom) =
//   splitAnnulus(outsideList, insideList, true, true); for (int i = 0; i < 10;
//   i++) {
//     std::cout << "TSA top " << top->coords.x << " " << top->coords.y << endl;
//     top = top->next;
//   }
//   for (int i = 0; i < 10; i++) {
//     std::cout << "TSA bottom " << bottom->coords.x << " " << bottom->coords.y
//               << endl;
//     bottom = bottom->next;
//   }
// }

void GHphase1(vertex *subject, vertex *clip) {
  vertex *subjectFirst = subject, *clipFirst = clip;
  vertex *subjectNext = subject, *clipNext = clip;
  float t, u;
  bool colinear; // in this version this is skipped
  vec2 point;
  cout << "1 segment intersect " << endl;
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
                           clipNext->coords, &t, &u, &colinear, &point)) {
        vertex *i1 = new vertex(point, t, true);
        vertex *i2 = new vertex(point, u, true);
        // insert to appropriate position between the vertices
        (*i1).insert(subject, subjectNext, i1);
        (*i2).insert(clip, clipNext, i2);
        // linking neighbor
        i1->neighbor = i2;
        i2->neighbor = i1;
        cout << "(" << point.x << "," << point.y << "),";
      }

      clip = clipNext;
    } while (clip != clipFirst);
    subject = subjectNext;
  } while (subject != subjectFirst);
}

// void GHphase2(vertex *subject, vertex *clip) {
//   // Going through the edges and set each intersecting
//   // points to either entry or exit
//   vertex *subjectFirst = subject;
//   vertex *clipFirst = clip;
//   bool entry = !pointInPolygon(clip, subject->coords);
//   // boolean flip the value to swap between intersection and difference
//   // for difference we will flip the value of entry
//   // however this will be moved to phase 3 to avoid computing phase 2 twice
//   // for the slight tradeoff of being a bit unintuitive
//   // entry = entry ^ difference;
//   cout << "2 pip " << clip->toString() << " (" << subject->coords.x << ", "
//        << subject->coords.y << ") " << entry << endl;
//   int timeout = 0;
//   do {
//     timeout++;
//     if (subject->intersect) {
//       subject->entry = entry;
//       entry = !entry;
//     }
//     subject = subject->next;
//     if (timeout > 1000) {
//       std::cerr << "Error timeout subject phase 2" << std::endl;
//       std::abort();
//     }
//   } while (subject != subjectFirst);
//   entry = !pointInPolygon(subject, clip->coords);
//   cout << "2 pip " << subject->toString() << " (" << clip->coords.x << ", "
//        << clip->coords.y << ") " << entry << endl;
//   timeout = 0;
//   do {
//     timeout++;
//     if (clip->intersect) {
//       clip->entry = entry;
//       entry = !entry;
//     }
//     clip = clip->next;
//     if (timeout > 1000) {
//       std::cerr << "Error timeout clip phase 2" << std::endl;
//       std::abort();
//     }
//   } while (clip != clipFirst);
// }

vertex *GHphase3(vertex *subject, vertex *clip, bool difference = false) {
  for (int i = 0; i < 10; i++) {
    // cout << "3 subject " << subject->coords.x << " " << subject->coords.y
    //<< endl;
    subject = subject->next;
  }
  for (int i = 0; i < 10; i++) {
    // cout << "3 clip " << clip->coords.x << " " << clip->coords.y << endl;
    clip = clip->next;
  }

  // walk through entry and exit points and construct clipped polygons
  vertex *current, *startPoly;
  vertex *start = subject;

  vertex *result = nullptr;
  vertex *resultHead = nullptr;
  // toggle to check if we are running in subject or clip polygon
  bool toggle;
  current = clip;
  startPoly = clip;
  // clear the processed values so that phase 3 can be done multiple times
  int testSubjectIntersection = 0;
  int testClipIntersection = 0;
  do {
    if (current->intersect) {
      testClipIntersection += 1;
    }
    current->processed = false;
    current = current->next;
  } while (current != startPoly);
  current = subject;
  startPoly = subject;
  do {
    if (current->intersect) {
      testSubjectIntersection += 1;
    }
    current->processed = false;
    current = current->next;
  } while (current != startPoly);
  int timeout = 0;
  while (timeout < 1000) {
    timeout++;
    if (timeout >= 1000) {
      std::cerr << "Error 3 timeout main" << std::endl;
      std::abort();
    }
    // find the intersecting point of subject polygon that isnt processed yet
    int processTimeout = 0;
    while (current->intersect == false || current->processed == true) {
      processTimeout += 1;
      if (processTimeout > 1000) {
        std::cerr << "Error 3 timeout finding" << std::endl;
        std::abort();
      }
      current = current->next;
      // cout << "TC 3 finding intersection " << endl;
      //  return if looping back to the start of polygon without finding next
      //  intersection point that is unprocessed
      if (startPoly->coords == current->coords) {
        cout << "TC 3 ended" << endl;
        return result;
      }
    }
    // denote the start of each sub-polygon of the intersection
    // most intersection will only create one polygon but not all
    start = current;
    toggle = true;
    vector<vec2> points;
    int dwTimeoutOut = 0;
    do {
      dwTimeoutOut++;
      if (dwTimeoutOut == 1000) {
        std::cerr << "Error: 3 timeout dw out" << std::endl;
        std::abort();
      }
      bool isEntry = current->label ^ (difference && toggle);
      int dwTimeoutIn = 0;
      do {
        dwTimeoutIn++;
        if (dwTimeoutIn == 1000) {
          std::cerr << "Error: 3 timeout dw in" << std::endl;
          std::abort();
        }
        current->processed = true;
        // Move forward at entry points and backward at exit points
        if (isEntry) {
          current = current->next;
        } else {
          current = current->prev;
        }
        points.push_back(current->coords);
      } while (!current->intersect);
      toggle = !toggle;
      // move to neighbor polygon after each streak move in one polygon
      current->processed = true;
      current = current->neighbor;
    } while (current->coords != start->coords); // repeat until polygon closed
    // build the polygon and connect to the last polygon
    result = buildPolygon(points);
    // for (int i = 0; i < 10; i++) {
    //   result = result->prev;
    //   cout << "33 result inside " << result << " " << result->toString() << "
    //   "
    //        << (result == nullptr) << endl;
    // }
    result->nextPoly = resultHead;
    resultHead = result;
  }
  std::cerr << "Error: 3 timeout" << std::endl;
  std::abort();
}

TURN checkTurn(vertex *pm, vertex *i, vertex *pp, vertex *q) {
  // q linked to p1
  if (pm->intersect && (pm->neighbor == q))
    return IS_PM;
  // q linked to p2
  if (pp->intersect && (pp->neighbor == q))
    return IS_PP;

  // check relative position of q
  double s1 = cross2D(pm->coords - q->coords, i->coords - q->coords);
  double s2 = cross2D(i->coords - q->coords, pp->coords - q->coords);
  double s3 = cross2D(i->coords - pm->coords, pp->coords - pm->coords);

  if (s3 > 0) {
    // left turn
    if (s1 > 0 && s2 > 0) {
      return LEFT;
    } else {
      return RIGHT;
    }
  } else {
    // right or straight turn
    if (s1 < 0 && s2 < 0) {
      return RIGHT;
    } else {
      return LEFT;
    }
  }
}

INTERSECT checkIntersectType(vertex *i, vertex *pm, vertex *pp, vertex *qm,
                             vertex *qp) {
  vec2 ic = i->coords;
  vec2 pmc = pm->coords, ppc = pp->coords;
  vec2 qmc = qm->coords, qpc = qp->coords;
  TURN qmType = checkTurn(pm, i, pp, qm);
  TURN qpType = checkTurn(pm, i, pp, qp);
  if (((qpType == IS_PP) && (qmType == RIGHT)) ||
      ((qmType == IS_PP) && (qpType == RIGHT)))
    return LEFT_ON;

  if (((qpType == IS_PP) && (qmType == LEFT)) ||
      ((qmType == IS_PP) && (qpType == LEFT)))
    return RIGHT_ON;

  if (((qpType == IS_PP) && (qmType == IS_PM)) ||
      ((qmType == IS_PP) && (qpType == IS_PM)))
    return ON_ON;

  if (((qmType == IS_PM) && (qpType == RIGHT)) ||
      ((qpType == IS_PM) && (qmType == RIGHT)))
    return ON_LEFT;

  if (((qmType == IS_PM) && (qpType == LEFT)) ||
      ((qpType == IS_PM) && (qmType == LEFT)))
    return ON_RIGHT;
  if ((qmType == LEFT && qpType == RIGHT) ||
      (qmType == RIGHT && qpType == LEFT)) {
    return CROSSING;
  }
  if ((qmType == LEFT && qpType == LEFT) ||
      (qmType == RIGHT && qpType == RIGHT)) {
    return BOUNCING;
  }
  return NONE;
};

#include <glm/glm.hpp>

void newGHphase1(vertex *subject, vertex *clip) {
  vertex *subjectFirst = subject, *clipFirst = clip;
  vertex *subjectNext = subject, *clipNext = clip;
  float t, u;
  bool colinear;
  vec2 point;
  cout << "1 segment intersect " << endl;
  do {
    subjectNext = subjectNext->next;
    while (subjectNext->inserted == true) {
      // continue passing all intersect points until it reach polygon vertex
      subjectNext = subjectNext->next;
    }
    do {
      clipNext = clipNext->next;
      while (clipNext->inserted == true) {
        // continue passing all intersect points until it reach polygon vertex
        clipNext = clipNext->next;
      }
      // P for subject polygon and q for clip polygon
      vec2 p1 = subject->coords, p2 = subjectNext->coords;
      vec2 q1 = clip->coords, q2 = clipNext->coords;
      bool intersect =
          segmentIntersect(p1, p2, q1, q2, &t, &u, &colinear, &point);
      if (intersect) {
        if (t > 0.0f && t < 1.0f && u > 0.0f && u < 1.0f) {
          // X intersection
          vertex *i1 = new vertex(point, t, true);
          vertex *i2 = new vertex(point, u, true);
          (*i1).insert(subject, subjectNext, i1);
          (*i2).insert(clip, clipNext, i2);
          // linking neighbor
          i1->neighbor = i2;
          i2->neighbor = i1;
        } else if (t == 0.0f && u == 0.0f) {
          // V intersection
          subject->neighbor = clip;
          clip->neighbor = subject;
          subject->intersect = true;
          clip->intersect = true;
        } else if (t == 0.0f && u > 0.0f && u < 1.0f) {
          // T intersection subject on clip
          vertex *i1 = new vertex(p1, u, true);
          // insert subject into coords and link
          (*i1).insert(clip, clipNext, i1);
          i1->neighbor = subject;
          subject->neighbor = i1;
          subject->intersect = true;
        } else if (u == 0.0f && t > 0.0f && t < 1.0f) {
          // T intersection clip on subject
          vertex *i1 = new vertex(q1, t, true);
          // insert subject into coords and link
          (*i1).insert(subject, subjectNext, i1);
          i1->neighbor = clip;
          clip->neighbor = i1;
          clip->intersect = true;
        }
      } else if (colinear) {
        // colinear cases
        float a, b;
        a = dot(q1 - p1, p2 - p1) / dot(p2 - p1, p2 - p1);
        b = dot(p1 - q1, q2 - q1) / dot(q2 - q1, q2 - q1);
        if (a > 0.0f && a < 1.0f && b > 0.0f && b < 1.0f) {
          // X overlap
          vertex *i1 = new vertex(p1, b, true);
          vertex *i2 = new vertex(q1, a, true);
          (*i1).insert(clip, clipNext, i1);
          (*i2).insert(subject, subjectNext, i2);
          // linking neighbor
          i1->neighbor = subject;
          subject->neighbor = i1;
          i2->neighbor = clip;
          clip->neighbor = i2;
          subject->intersect = true;
          clip->intersect = true;
        } else if (a == 0.0f && b == 0.0f) {
          // V overlap
          subject->neighbor = clip;
          clip->neighbor = subject;
          subject->intersect = true;
          clip->intersect = true;
        } else if ((a < 0.0f || a >= 1) && b > 0.0f && b < 1.0f) {
          // T overlap subject on clip
          vertex *i1 = new vertex(p1, b, true);
          // insert subject into coords and link
          (*i1).insert(clip, clipNext, i1);
          i1->neighbor = subject;
          subject->neighbor = i1;
          subject->intersect = true;
        } else if ((b < 0.0f || b >= 1) && a > 0.0f && a < 1.0f) {
          // T overlap clip on subject
          vertex *i1 = new vertex(q1, a, true);
          // insert subject into coords and link
          (*i1).insert(subject, subjectNext, i1);
          i1->neighbor = clip;
          clip->neighbor = i1;
          clip->intersect = true;
        }
      }

      clip = clipNext;
    } while (clip != clipFirst);
    subject = subjectNext;
  } while (subject != subjectFirst);
}

void newGHphase2(vertex *subject, vertex *clip) {
  bool UNION = false;
  vertex *subjectFirst = subject;
  vertex *clipFirst = clip;
  // timeout is an artifact from the old GH implementation to detect problems
  // such as infinite loops. probably not needed right now, just leave it
  // loop through subject list and clip list, check entry types

  // phase 2.1
  for (int i = 0; i < 2; i++) {
    vertex *first = i == 0 ? subject : clip;
    vertex *current = first;
    do {
      if (current->intersect) {
        vertex *pm = current->prev;           // p- vertex
        vertex *pp = current->next;           // p+ vertex
        vertex *qm = current->neighbor->prev; // q- vertex
        vertex *qp = current->neighbor->next; // q+ vertex
        current->label = checkIntersectType(current, pm, pp, qm, qp);
      }
      current = current->next;
    } while (current != first);
  }

  subjectFirst = subject;
  // phase 2.2
  // loop through subject list, only taking the intersection chains to modify
  // and labelling the intersection chains
  do {
    if (subject->label == LEFT_ON || subject->label == RIGHT_ON) {
      // remember status of the first chain vertex and vertex itself
      TURN x = subject->label == LEFT_ON ? LEFT : RIGHT;
      vertex *X = subject;

      // proceed to end of the chain and mark all visited vertices as NONE
      do {
        subject->label = NONE;
        subject = subject->next;
      } while (subject->label == ON_ON);

      TURN y = subject->label == ON_LEFT ? LEFT : RIGHT;

      // determine type of intersection chain
      INTERSECT chainType;
      if (x != y) {
        chainType = DELAYED_CROSSING;
      } else {
        chainType = DELAYED_BOUNCING;
      }

      // mark both ends of an intersection chain with chainType
      X->label = chainType;
      subject->label = chainType;
    }
    subject = subject->next;
  } while (subject != subjectFirst);

  // 2.3 copy from p to q
  subjectFirst = subject;
  do {
    if (subject->intersect) {
      subject->neighbor->label = subject->label;
    }
    subject = subject->next;
  } while (subject != subjectFirst);

  // phase 2.4
  // loop through subject list and clip list for entry/exit marking
  for (int i = 0; i < 2; i++) {
    vertex *first = i == 0 ? subject : clip;
    vertex *current = first;
    vertex *opposite = first == subject ? clip : subject;
    do {
      current = current->next;
    } while (current->intersect);
    // here means that found a vertex from P/Q that doesnt intersect with Q/P

    // need to verify the state of this
    ENTRYEXIT status = pointInPolygon(opposite, current->coords) ? ENTRY : EXIT;
    bool firstChainVertex = true;

    vertex *innerCurrent = current;
    do {
      if (innerCurrent->label == CROSSING) {
        innerCurrent->entry = status;
        status = status == ENTRY ? EXIT : ENTRY;
      }
      if (innerCurrent->label == BOUNCING && (status == EXIT) ^ UNION) {
        // TODO: what is split vertex candidates, add later
      }
      // for delayed crossing, mark both endpoints with entry/exit status
      // toggle at the last end
      // in case of delayed exit, first vertex mark as crossing
      // in case of delayed entry, last vertex mark as crossing
      if (innerCurrent->label == DELAYED_CROSSING) {
        innerCurrent->entry = status;
        if (firstChainVertex) { // is this first vertex of delayed chain
          if ((status == EXIT) ^ UNION) {
            innerCurrent->label = CROSSING;
          }
        } else {
          if ((status == ENTRY) ^ UNION) {
            innerCurrent->label = CROSSING;
          }
        }
        status = status == ENTRY ? EXIT : ENTRY;
        firstChainVertex = !firstChainVertex;
      }
      // for delayed bouncing, mark both end with entry/exit status
      // toggle on either, or none (?)
      if (innerCurrent->label == DELAYED_BOUNCING) {
        innerCurrent->entry = status;
        if (firstChainVertex) { // is this first vertex of delayed chain
          if ((status == EXIT) ^ UNION) {
            // TODO: mark first exit vertex as crossing candidate (?)
          }
        } else {
          if ((status == ENTRY) ^ UNION) {
            // TODO: mark last entry vertex as crossing candidate (?)
          }
        }
        status = status == ENTRY ? EXIT : ENTRY;
        firstChainVertex = !firstChainVertex;
      }
      innerCurrent = innerCurrent->next;
    } while (innerCurrent != current);
  }
}

vertex *newGHphase3(vertex *subject, vertex *clip, bool difference = false) {
  for (int i = 0; i < 10; i++) {
    // cout << "3 subject " << subject->coords.x << " " << subject->coords.y
    //<< endl;
    subject = subject->next;
  }
  for (int i = 0; i < 10; i++) {
    // cout << "3 clip " << clip->coords.x << " " << clip->coords.y << endl;
    clip = clip->next;
  }

  // walk through entry and exit points and construct clipped polygons
  vertex *current, *startPoly;
  vertex *start = subject;

  vertex *result = nullptr;
  vertex *resultHead = nullptr;
  // toggle to check if we are running in subject or clip polygon
  bool toggle;
  current = clip;
  startPoly = clip;
  // clear the processed values so that phase 3 can be done multiple times
  int testSubjectIntersection = 0;
  int testClipIntersection = 0;
  do {
    if (current->intersect) {
      testClipIntersection += 1;
    }
    current->processed = false;
    current = current->next;
  } while (current != startPoly);
  current = subject;
  startPoly = subject;
  do {
    if (current->intersect) {
      testSubjectIntersection += 1;
    }
    current->processed = false;
    current = current->next;
  } while (current != startPoly);
  int timeout = 0;
  while (timeout < 1000) {
    timeout++;
    if (timeout >= 1000) {
      std::cerr << "Error 3 timeout main" << std::endl;
      std::abort();
    }
    // find the intersecting point of subject polygon that isnt processed yet
    int processTimeout = 0;
    while (current->intersect == false || current->processed == true) {
      processTimeout += 1;
      if (processTimeout > 1000) {
        std::cerr << "Error 3 timeout finding" << std::endl;
        std::abort();
      }
      current = current->next;
      // cout << "TC 3 finding intersection " << endl;
      //  return if looping back to the start of polygon without finding next
      //  intersection point that is unprocessed
      if (startPoly->coords == current->coords) {
        cout << "TC 3 ended" << endl;
        return result;
      }
    }
    // denote the start of each sub-polygon of the intersection
    // most intersection will only create one polygon but not all
    start = current;
    toggle = true;
    vector<vec2> points;
    int dwTimeoutOut = 0;
    do {
      dwTimeoutOut++;
      if (dwTimeoutOut == 1000) {
        std::cerr << "Error: 3 timeout dw out" << std::endl;
        std::abort();
      }
      bool isEntry = current->entry ^ (difference && toggle);
      int dwTimeoutIn = 0;
      do {
        dwTimeoutIn++;
        if (dwTimeoutIn == 1000) {
          std::cerr << "Error: 3 timeout dw in" << std::endl;
          std::abort();
        }
        current->processed = true;
        // Move forward at entry points and backward at exit points
        if (isEntry) {
          current = current->next;
        } else {
          current = current->prev;
        }
        points.push_back(current->coords);
      } while (!current->intersect);
      toggle = !toggle;
      // move to neighbor polygon after each streak move in one polygon
      current->processed = true;
      current = current->neighbor;
    } while (current->coords != start->coords); // repeat until polygon closed
    // build the polygon and connect to the last polygon
    result = buildPolygon(points);
    // for (int i = 0; i < 10; i++) {
    //   result = result->prev;
    //   cout << "33 result inside " << result << " " << result->toString() << "
    //   "
    //        << (result == nullptr) << endl;
    // }
    result->nextPoly = resultHead;
    resultHead = result;
  }
  std::cerr << "Error: 3 timeout" << std::endl;
  std::abort();
}

// // take in projected beam and face their x and y component
// // return difference first and then intersection
// std::tuple<vector<vertex *>, vector<vertex *>>
// polygonClipping(vector<vec3> beam, vector<vec3> face) {
//   // assume each polygons have at least 3 vertices which is non degenerate
//   vec3 normal = cross(face[1] - face[0], face[2] - face[0]);
//   vec3 origin = face[0];
//   cout << "poly beam polygon(";
//   for (vec3 b : beam) {
//     cout << "(" << b.x << "," << b.y << "),";
//   }
//   cout << ")" << endl;
//   cout << "poly face polygon(";
//   for (vec3 f : face) {
//     cout << "(" << f.x << "," << f.y << "),";
//   }
//   cout << ")" << endl;
//   vec3 *intersection = new vec3(0, 0, 0);
//   vector<vec2> subject2D = {};
//   vector<vec2> clip2D = {};
//   vertex *subjectList;
//   vertex *clipList;
//   for (int i = 0; i < beam.size(); i++) {
//     subject2D.push_back(vec2(beam[i].x, beam[i].y));
//   }
//   for (int i = 0; i < face.size(); i++) {
//     clip2D.push_back(vec2(face[i].x, face[i].y));
//   }
//   cout << "poly done to 2d " << subject2D.size() << " " << clip2D.size()
//        << endl;
//   subjectList = buildPolygon(subject2D);
//   clipList = buildPolygon(clip2D);
//   for (int i = 0; i < 10; i++) {
//     // cout << "poly 0 subject " << subjectList << " " <<
//     // subjectList->toString()
//     //      << endl;
//     subjectList = subjectList->next;
//   }
//   for (int i = 0; i < 10; i++) {
//     // cout << "poly 0 clip " << clipList << " " << clipList->toString() <<
//     // endl;
//     clipList = clipList->next;
//   }
//   GHphase1(subjectList, clipList);
//   GHphase2(subjectList, clipList);
//   // TODO: find a solution for modifying beam so that it support fragmented
//   beam vertex *resultBeam = GHphase3(subjectList, clipList, true); vertex
//   *result = GHphase3(subjectList, clipList);
//   // intersection results
//   vertex *resultHead = result;
//   if (result == nullptr) {
//     if (pointInPolygon(subjectList, clipList->coords)) {
//       cout << "poly no intersection clip inside " << resultBeam << " " <<
//       result
//            << endl;
//       // beam is holed polygon and need to clip
//       bool outer = polygonClockwise(subjectList);
//       bool inner = polygonClockwise(clipList);

//       vector<vertex *> b;
//       vector<vertex *> r;
//       std::tie(b, r) = splitAnnulus(subjectList, clipList, outer, inner);

//       cout << "poly no intersection annulus " << b.size() << " " << r.size()
//            << endl;
//       for (int i = 0; i < 10; i++) {
//         b[0] = b[0]->next;
//         std::cout << "poly beam annulus result " << b[0]->coords.x << " "
//                   << b[0]->coords.y << endl;
//       }
//       for (int i = 0; i < 10; i++) {
//         b[1] = b[1]->next;
//         std::cout << "poly beam annulus result " << b[1]->coords.x << " "
//                   << b[1]->coords.y << endl;
//       }
//       for (int i = 0; i < 10; i++) {
//         r[0] = r[0]->next;
//         std::cout << "poly face annulus result " << r[0]->coords.x << " "
//                   << r[0]->coords.y << endl;
//       }
//       return {b, r};
//     } else if (pointInPolygon(clipList, subjectList->coords)) {
//       cout << "poly no intersection clip outside" << endl;
//       // beam lost and not propagate further, the entire beam bounces
//       return {{}, {subjectList}};
//     } else {
//       cout << "poly no intersection disjoint" << endl;
//       // no intersection, the full beam propagate further
//       return {{subjectList}, {}};
//     }
//   }
//   return {{resultBeam}, {result}};
// }

// vector<vec2> testClipping(vector<vec3> subject, vector<vec3> clip) {
//   cout << "test 00 " << subject.size() << " " << clip.size() << endl;
//   vec3 normal = cross(clip[1] - clip[0], clip[2] - clip[0]);
//   vec3 origin = clip[0];
//   vec3 *intersection = new vec3(0, 0, 0);
//   vector<vec2> subject2D = {};
//   vector<vec2> clip2D = {};
//   vertex *subjectList;
//   vertex *clipList;
//   for (int i = 0; i < subject.size(); i++) {
//     subject2D.push_back(vec2(subject[i].x, subject[i].y));
//   }
//   for (int i = 0; i < clip.size(); i++) {
//     clip2D.push_back(vec2(clip[i].x, clip[i].y));
//   }
//   subjectList = buildPolygon(subject2D);
//   clipList = buildPolygon(clip2D);
//   // cout << "test 0" << endl;
//   GHphase1(subjectList, clipList);
//   // cout << "test 1" << endl;
//   GHphase2(subjectList, clipList);
//   // cout << "test 2" << endl;
//   vertex *result = GHphase3(subjectList, clipList, true);
//   //  cout << "test 3" << endl;
//   vertex *resultHead = result;
//   if (result == nullptr) {
//     return {};
//   }
//   vector<vec2> resultCoords = {};
//   for (int i = 0; i < 100; i++) {
//     resultCoords.push_back(result->coords);
//     result = result->next;
//     if (result == resultHead) {
//       if (result->nextPoly != nullptr) {
//         result = result->nextPoly;
//         resultCoords.push_back(vec2(INFINITY, INFINITY));
//         resultHead = result;
//         cout << endl;
//       } else {
//         break;
//       }
//     }
//   }
//   resultCoords.push_back(vec2(INFINITY, INFINITY));
//   return resultCoords;
// }

// Testing out this matrix code
mat3 makePrimaryMatrix() {
  // TODO: calculate from camera
  vec3 cameraF = vec3(0, 0, 1);
  vec3 cameraR = vec3(1, 0, 0);
  vec3 cameraU = vec3(0, 1, 0);
  vec3 row0 = 0.5f * cameraF + 0.5f * cameraR;
  vec3 row1 = 0.5f * cameraF + 0.5f * cameraU;
  vec3 row2 = cameraF;
  return transpose(mat3x3(row0, row1, row2)); // convert to rows
}
mat3 makeChildMatrix(mat3x3 parent, vec3 n) {
  return parent * (mat3x3(1.0f) - 2.0f * outerProduct(n, n));
}

mat3 makeHMatrix(mat3x3 M, vec3 u, vec3 v, vec3 d) {
  return mat3(M * u, M * v, M * d);
}

inline vec2 beamToUV(mat3 H, vec2 point) {
  vec3 t = H * vec3(point, 1.0f);
  return vec2(t.x / t.z, t.y / t.z);
}

void traceBeams(vec3 apex = vec3(0, 0, 0), vec3 d = vec3(0, 0, 1),
                vector<vec3> polygon = {vec3(-1, -1, 1), vec3(-1, 1, 1),
                                        vec3(1, 1, 1), vec3(1, -1, 1)},
                mat3x3 matrix = makePrimaryMatrix()) {
  vector<vector<vec3>> crossSection = {{}};
  int objectIndex;
  mesh m = getMesh(0);
  // save a copy to mutate
  vector<vec3> vertices;
  vector<int> indices;
  vector<color> colors;
  vec3 u = vec3(0, 0, 0), v = vec3(0, 0, 0);
  // find u by crossing with least aligned axis
  vec3 ad = abs(d);
  vec3 a = vec3(ad.x <= ad.y && ad.x <= ad.z, ad.x > ad.y && ad.y <= ad.z,
                ad.x > ad.z && ad.y > ad.z);
  u = cross(a, d);
  v = cross(u, d);
  cout << "PC UV " << printVec(u) << " " << printVec(v) << endl;
  vec3 vert[3];
  color col = color::base::black;
  // projecting cross section to beam space
  for (int i = 0; i < polygon.size(); i++) {
    vec3 p = polygon[i] - apex;
    float z = dot(p, d);
    crossSection[0].push_back(vec3(dot(p, u) / z, dot(p, v) / z, z));
  }
  // get a copy of all triangles to modify
  for (int i = 0; i < getSceneSize(); i++) {
    m = getMesh(i);
    vector<vec3> tempVertices = m.getVertices();
    vector<int> tempIndices = m.getIndices();
    vector<color> tempColors = m.getVertexColors();
    vertices.insert(vertices.end(), tempVertices.begin(), tempVertices.end());
    indices.insert(indices.end(), tempIndices.begin(), tempIndices.end());
    colors.insert(colors.end(), tempColors.begin(), tempColors.end());
  }
  cout << "PC before vertices " << endl;
  // projecting the vertices to beam space
  for (int i = 0; i < vertices.size(); i++) {
    vec3 p = vertices[i] - apex;
    float z = dot(p, d);
    vertices[i] = vec3(dot(p, u) / z, dot(p, v) / z, z);
  }
  cout << "PC done all projection " << endl;
  // sort the mesh front to back
  vector<float> minZ(indices.size() / 3);
  for (int j = 0; j < indices.size() / 3; j++) {
    minZ[j] = std::min(
        std::min(vertices[indices[3 * j]].z, vertices[indices[3 * j + 1]].z),
        vertices[indices[3 * j + 2]].z);
  }
  // sort triples of indices by their corresponding minZ
  vector<int> order(indices.size() / 3);
  vector<int> newIndices(indices.size());
  iota(order.begin(), order.end(), 0);
  sort(order.begin(), order.end(),
       [&](int a, int b) { return minZ[a] < minZ[b]; });
  for (int i = 0; i < order.size(); i++) {
    newIndices[i * 3] = indices[order[i] * 3];
    newIndices[i * 3 + 1] = indices[order[i] * 3 + 1];
    newIndices[i * 3 + 2] = indices[order[i] * 3 + 2];
  }
  for (int i = 0; i < newIndices.size() / 3; i++) {
    cout << "PC order " << printVec(vertices[newIndices[3 * i]]) << " "
         << printVec(vertices[newIndices[3 * i + 1]]) << " "
         << printVec(vertices[newIndices[3 * i + 2]]) << " " << endl;
  }
  // process each triangle
  // note: crossSection is of type vector<vec3>
  // both crossSection and vertices are modified to be in term of d, u, v
  // where d, u, v is vector forward and traverse relative to bea,
  // polygons are represented by linked lists of vertices
  mat3x3 H = makeHMatrix(matrix, u, v, d);
  for (int i = 0; i < newIndices.size() / 3; i++) {
    vector<vertex *> resultBeam;
    vector<vertex *> result;
    // TODO: make a function to turn the cross section back to beams
    int crossID = crossSection.size() - 1;
    vector<vector<vec3>> newCross = {}; // a copy of cross section to add in
    while (crossID >= 0) {
      std::tie(resultBeam, result) = polygonClipping(
          crossSection[crossID], vector<vec3>{vertices[newIndices[3 * i]],
                                              vertices[newIndices[3 * i + 1]],
                                              vertices[newIndices[3 * i + 2]]});
      cout << "PC result beam and face size " << resultBeam.size() << " "
           << result.size() << endl;
      for (int j = 0; j < resultBeam.size(); j++) {
        std::vector<vec3> newCrossSection = {};
        vertex *currentResult = resultBeam[j];
        vertex *headResult = currentResult;
        // convert resultBeam to 3d
        // here check if this formula is correct
        do {
          vec3 projectedBeam =
              vec3(currentResult->coords.x, currentResult->coords.y, 1);
          currentResult = currentResult->next;
          newCrossSection.push_back(projectedBeam);
        } while (currentResult != headResult);
        newCross.push_back(newCrossSection);
      }
      // cross section set to resultBeam convert to 3d
      for (int j = 0; j < result.size(); j++) {
        vertex *currentResult = result[j];
        vertex *headResult = currentResult;
        vector<vec2> polygon = {};
        do {
          // using H matrix made by combining reflection bounces
          vec2 uv = beamToUV(H, currentResult->coords);
          uv = vec2(uv.x * getScreenResolutionX(),
                    uv.y * getScreenResolutionY());
          polygon.push_back(uv);
          cout << "PC result polygon " << uv.x << " " << uv.y << endl;
          currentResult = currentResult->next;
          // TODO: render this on the screen like raster
        } while (currentResult != headResult);
        cout << "PC start raster" << endl;
        raster2DPolygon(polygon);
      }

      crossID--;
    }
    crossSection = newCross;
  }
}

void traceBeams() { traceBeams(getCameraPosition()); };

// int main2() {
//   // vector<BRay> beam;
//   // beam.push_back(BRay(vec3(0, 0, 1), vec3(-1, 1, 1)));
//   // beam.push_back(BRay(vec3(0, 0, 1), vec3(1, 1, 1)));
//   // beam.push_back(BRay(vec3(0, 0, 1), vec3(1, 0, 1)));
//   // beam.push_back(BRay(vec3(0, 0, 1), vec3(-1, 0, 1)));

//   vector<vec3> tempv = {
//       vec3(-1, 1, 1) + vec3(0, 0, 5),   vec3(1, 1, 1) + vec3(0, 0, 5),
//       vec3(-1, 1, -1) + vec3(0, 0, 5),  vec3(1, 1, -1) + vec3(0, 0, 5),
//       vec3(-1, -1, 1) + vec3(0, 0, 5),  vec3(1, -1, 1) + vec3(0, 0, 5),
//       vec3(-1, -1, -1) + vec3(0, 0, 5), vec3(1, -1, -1) + vec3(0, 0, 5)};
//   int tempi[36] = {0, 1, 2, 1, 3, 2, 6, 5, 4, 6, 7, 5, 0, 5, 1, 0, 4, 5,
//                    2, 3, 6, 3, 7, 6, 0, 2, 4, 2, 6, 4, 1, 5, 3, 3, 5, 7};
//   for (int i = 0; i < 12; i++) {
//     cout << "polygon clipping" << endl;
//     // polygonClipping(beam, vector<vec3>{tempv[2], tempv[3],
//     // tempv[6]});
//     // polygonClipping(beam, vector<vec3>{tempv[tempi[3 * i]],
//     //                                         tempv[tempi[3 * i + 1]],
//     //                                         tempv[tempi[3 * i + 2]]});
//   }
//   return 0;
//   vector<vec2> subjectData = {vec2(1, 3), vec2(1, 7), vec2(7, 7), vec2(7,
//   3)}; vector<vec2> clipData = {vec2(2, 1), vec2(2, 5), vec2(4, 2),
//   vec2(6.5f, 4.5f),
//                            vec2(6.5f, 1.5f)};
//   // reverse(clipData.begin(), clipData.end());
//   vertex *subject = buildPolygon(subjectData);
//   vertex *clip = buildPolygon(clipData);
//   vertex *subjectHead = subject;
//   vertex *clipHead = clip;
//   cout << "First subject " << subject << " " << subject->toString() << endl;

//   cout << "In polygon check " << pointInPolygon(clip, vec2(3, 3)) << endl;

//   for (int i = 0; i < 10; i++) {
//     cout << "0 subject " << subject << " " << subject->toString() << endl;
//     subject = subject->next;
//   }
//   for (int i = 0; i < 10; i++) {
//     cout << "0 clip " << clip << " " << clip->toString() << endl;
//     clip = clip->next;
//   }

//   GHphase1(subject, clip);

//   for (int i = 0; i < 10; i++) {
//     cout << "1 subject " << subject << " " << subject->toString() << endl;
//     subject = subject->next;
//   }
//   for (int i = 0; i < 10; i++) {
//     cout << "1 clip " << clip << " " << clip->toString() << endl;
//     clip = clip->next;
//   }
//   subject = subjectHead;
//   clip = clipHead;
//   GHphase2(subject, clip);
//   for (int i = 0; i < 10; i++) {
//     cout << "2 subject " << subject << " " << subject->toString() << " "
//          << subject->entry << endl;
//     subject = subject->next;
//   }
//   for (int i = 0; i < 10; i++) {
//     cout << "2 clip " << clip << " " << clip->toString() << " " <<
//     clip->entry
//          << endl;
//     clip = clip->next;
//   }
//   vertex *result = GHphase3(subject, clip);
//   vertex *resultHead = result;
//   for (int i = 0; i < 10; i++) {

//     cout << "3 result " << result << " " << result->toString() << " "
//          << (result == nullptr) << endl;
//     result = result->next;
//     if (result == resultHead) {
//       if (result->nextPoly != nullptr) {
//         result = result->nextPoly;
//         resultHead = result;
//         cout << endl;
//       } else {
//         break;
//       }
//     }
//   }
//   return 0;
// }
