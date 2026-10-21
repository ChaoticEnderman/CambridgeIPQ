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
#include "renderer.h"

using namespace glm;
using namespace std;

const float epsilon = 0.0001f;
// float camLen = 0.1f;

inline float cross2D(vec2 ab, vec2 ac) { return ab.x * ac.y - ab.y * ac.x; }

static inline int sgn(int64_t v) { return (v > 0) - (v < 0); }

// mat4x4 cameraMat = mat4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

string printVec(vec3 v) {
  return "(" + to_string(v.x) + " " + to_string(v.y) + " " + to_string(v.z) +
         ")";
}

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
  return *t > 0.0f && *u >= 0.0f && *u < 1.0f;
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

  string toString() {
    // for testing purposes only
    return "[(" + to_string(coords.x) + ", " + to_string(coords.y) +
           ") intersect " + (intersect ? "true" : "false") + " entry " +
           (entry ? "true" : "false") + " alpha " + to_string(alpha) + "]";
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

// split a ring shape into two concave polygons
vertex *splitAnnulus(vertex *outside, vertex *inside) {
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

  // building polygon
  outsideHead = outside;
  insideHead = inside;
  bool outerPolygon = true;
  bool jumpInLeft;
  vertex *top = outside;
  vertex *topHead = top;
  // move in or out at each intersection
  do {
    if (outside == leftEdge && outerPolygon) {
      outside = innerLeftEdge;
      outerPolygon = !outerPolygon;
      // jump in from the left, only out from the right
      jumpInLeft = true;
    } else if ((outside == innerLeftEdge && !outerPolygon)) {
      outside = insertLeft;
      outerPolygon = !outerPolygon;
    } else if (outside == rightEdge && outerPolygon) {
      outside = innerRightEdge;
      outerPolygon = !outerPolygon;
      // jump in from the right, only out from the left
      jumpInLeft = false;
    } else if ((outside == innerRightEdge && !outerPolygon)) {
      outside = insertRight;
      outerPolygon = !outerPolygon;
    } else {
      outside = outside->next;
      top->next = outside;
      outside->prev = top;
      top = top->next;
    }
  } while (outside != outsideHead);
  top->next = topHead;
  topHead->prev = top;
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
  bool entry = !pointInPolygon(clip, subject->coords);
  // boolean flip the value to swap between intersection and difference
  // for difference we will flip the value of entry
  // however this will be moved to phase 3 to avoid computing phase 2 twice
  // for the slight tradeoff of being a bit unintuitive
  // entry = entry ^ difference;
  cout << "2 pip " << clip->toString() << " (" << subject->coords.x << ", "
       << subject->coords.y << ") " << entry << endl;
  do {
    if (subject->intersect) {
      subject->entry = entry;
      entry = !entry;
    }
    subject = subject->next;
  } while (subject != subjectFirst);
  entry = !pointInPolygon(subject, clip->coords);
  cout << "2 pip " << subject->toString() << " (" << clip->coords.x << ", "
       << clip->coords.y << ") " << entry << endl;
  do {
    if (clip->intersect) {
      clip->entry = entry;
      entry = !entry;
    }
    clip = clip->next;
  } while (clip != clipFirst);
}

vertex *GHphase3(vertex *subject, vertex *clip, bool difference = false) {
  // walk through entry and exit points and construct clipped polygons
  vertex *current = subject;
  vertex *startPoly = subject;
  vertex *start = subject;

  vertex *result = nullptr;
  vertex *resultHead = nullptr;
  // toggle to check if we are running in subject or clip polygon
  bool toggle;
  do {
    current->processed = false;
    current = current->next;
  } while (current != startPoly);
  while (true) {
    // find the intersecting point of subject polygon that isnt processed yet
    while (current->intersect == false || current->processed == true) {
      current = current->next;
      // return if looping back to the start of polygon without finding next
      // intersection point that is unprocessed
      if (startPoly->coords == current->coords) {
        cout << "3 ended" << endl;
        return result;
      }
    }
    // denote the start of each sub-polygon of the intersection
    // most intersection will only create one polygon but not all
    start = current;
    toggle = true;
    vector<vec2> points;
    do {
      bool isEntry = current->entry ^ (difference && toggle);
      do {
        current->processed = true;
        // Move forward at entry points and backward at exit points
        if (isEntry) {
          current = current->next;
        } else {
          current = current->prev;
        }
        points.push_back(current->coords);
        cout << "3 push back " << to_string(current->coords.x) << " "
             << to_string(current->coords.y) << " " << points.size() << endl;
      } while (!current->intersect);
      toggle = !toggle;
      // move to neighbor polygon after each streak move in one polygon
      current->processed = true;
      current = current->neighbor;
    } while (current->coords != start->coords); // repeat until polygon closed
    // build the polygon and connect to the last polygon
    result = buildPolygon(points);
    for (int i = 0; i < 10; i++) {
      result = result->prev;
      cout << "33 result inside " << result << " " << result->toString() << " "
           << (result == nullptr) << endl;
    }
    result->nextPoly = resultHead;
    resultHead = result;
  }
}

// clip a projected beam polygon against mesh faces
// will mutate the beam by subtraction (can leave empty beam)
// return a list of polygons intersection
std::tuple<vertex *, vertex *> polygonClipping(vector<vec3> beam,
                                               vector<vec3> face) {
  // assume each polygons have at least 3 vertices which is non degenerate
  vec3 normal = cross(face[1] - face[0], face[2] - face[0]);
  vec3 origin = face[0];
  cout << "poly face 0 " << face[0].x << " " << face[0].y << " " << face[0].z
       << endl;
  cout << "poly face 1 " << face[1].x << " " << face[1].y << " " << face[1].z
       << endl;
  cout << "poly face 2 " << face[2].x << " " << face[2].y << " " << face[2].z
       << endl;
  vec3 *intersection = new vec3(0, 0, 0);
  vector<vec2> subject2D = {};
  vector<vec2> clip2D = {};
  vertex *subjectList;
  vertex *clipList;
  for (int i = 0; i < beam.size(); i++) {
    subject2D.push_back(vec2(beam[i].x, beam[i].y));
  }
  for (int i = 0; i < face.size(); i++) {
    clip2D.push_back(vec2(face[i].x, face[i].y));
  }
  cout << "poly done to 2d " << subject2D.size() << " " << clip2D.size()
       << endl;
  subjectList = buildPolygon(subject2D);
  clipList = buildPolygon(clip2D);
  for (int i = 0; i < 10; i++) {
    cout << "poly 0 subject " << subjectList << " " << subjectList->toString()
         << endl;
    subjectList = subjectList->next;
  }
  for (int i = 0; i < 10; i++) {
    cout << "poly 0 clip " << clipList << " " << clipList->toString() << endl;
    clipList = clipList->next;
  }
  GHphase1(subjectList, clipList);
  GHphase2(subjectList, clipList);
  // TODO: find a solution for modifying beam so that it support fragmented beam
  vertex *resultBeam = GHphase3(subjectList, clipList, true);
  vertex *result = GHphase3(subjectList, clipList);
  // intersection results
  vertex *resultHead = result;
  if (result == nullptr) {
    if (pointInPolygon(subjectList, clipList->coords)) {
      cout << "poly no intersection clip inside" << endl;
      // beam is holed polygon and need to clip
      // TODO: clip inside so holed polygon and need to divide
      // return buildPolygon(clip2D);
      return {resultBeam, resultBeam};
    } else if (pointInPolygon(clipList, subjectList->coords)) {
      cout << "poly no intersection clip outside" << endl;
      // beam lost and not propagate further, the entire beam bounces
      return {nullptr, resultBeam};
    } else {
      cout << "poly no intersection disjoint" << endl;
      // no intersection, the full beam propagate further
      return {resultBeam, nullptr};
    }
  }
  for (int i = 0; i < 10; i++) {
    // vec3 temp = vector2To3(u, v, origin, result->coords);
    // cout << "poly result " << temp.x << " " << temp.y << " " << temp.z
    //           << endl;
    result = result->next;
    if (result == resultHead) {
      if (result->nextPoly != nullptr) {
        result = result->nextPoly;
        resultHead = result;
        cout << endl;
      } else {
        break;
      }
    }
  }
  return {resultBeam, result};
}

vector<vec2> testClipping(vector<vec3> subject, vector<vec3> clip) {
  cout << "test 00 " << subject.size() << " " << clip.size() << endl;
  vec3 normal = cross(clip[1] - clip[0], clip[2] - clip[0]);
  vec3 origin = clip[0];
  cout << "poly clip 0 " << clip[0].x << " " << clip[0].y << " " << clip[0].z
       << endl;
  cout << "poly clip 1 " << clip[1].x << " " << clip[1].y << " " << clip[1].z
       << endl;
  cout << "poly clip 2 " << clip[2].x << " " << clip[2].y << " " << clip[2].z
       << endl;
  vec3 *intersection = new vec3(0, 0, 0);
  vector<vec2> subject2D = {};
  vector<vec2> clip2D = {};
  vertex *subjectList;
  vertex *clipList;
  for (int i = 0; i < subject.size(); i++) {
    subject2D.push_back(vec2(subject[i].x, subject[i].y));
  }
  for (int i = 0; i < clip.size(); i++) {
    clip2D.push_back(vec2(clip[i].x, clip[i].y));
  }
  subjectList = buildPolygon(subject2D);
  clipList = buildPolygon(clip2D);
  // cout << "test 0" << endl;
  GHphase1(subjectList, clipList);
  // cout << "test 1" << endl;
  GHphase2(subjectList, clipList);
  // cout << "test 2" << endl;
  vertex *result = GHphase3(subjectList, clipList, true);
  result = GHphase3(subjectList, clipList, true);
  // cout << "test 3" << endl;
  vertex *resultHead = result;
  if (result == nullptr) {
    return {};
  }
  vector<vec2> resultCoords = {};
  for (int i = 0; i < 10; i++) {
    cout << "poly result " << result->coords.x << " " << result->coords.y
         << endl;
    resultCoords.push_back(result->coords);
    result = result->next;
    if (result == resultHead) {
      if (result->nextPoly != nullptr) {
        result = result->nextPoly;
        resultCoords.push_back(vec2(INFINITY, INFINITY));
        resultHead = result;
        cout << endl;
      } else {
        break;
      }
    }
  }
  resultCoords.push_back(vec2(INFINITY, INFINITY));
  return resultCoords;
}

void traceBeams(vec3 apex = vec3(0, 0, 0), vec3 d = vec3(0, 0, 1),
                vector<vec2> polygon = {vec2(-1, -1), vec2(-1, 1), vec2(1, 1),
                                        vec2(1, -1)}) {
  vector<vec3> crossSection;
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
    vec3 p = vec3(polygon[i].x, polygon[i].y, 1);
    crossSection.push_back(vec3(dot(p, u), dot(p, v), dot(p, d)));
    cout << "PC projected cross section " << p.x << " " << p.y << " " << p.z
         << endl;
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
  // projecting the vertices to beam space
  for (int i = 0; i < vertices.size(); i++) {
    vec3 p = vertices[i] - apex;
    vertices[i] = vec3(dot(p, u), dot(p, v), dot(p, d));
    float rd = 1.0f / vertices[i].z;
    vertices[i] = vec3(vertices[i].x * rd, vertices[i].y * rd, vertices[i].z);
  }
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
  // for (int i = 0; i < newIndices.size() / 3; i++) {
  int i = 6;
  cout << "PC hitting first triangle? " << printVec(vertices[newIndices[3 * i]])
       << " " << printVec(vertices[newIndices[3 * i + 1]]) << " "
       << printVec(vertices[newIndices[3 * i + 2]]) << endl;
  polygonClipping(crossSection, vector<vec3>{vertices[newIndices[3 * i]],
                                             vertices[newIndices[3 * i + 1]],
                                             vertices[newIndices[3 * i + 2]]});
  // break;
  // }
}
int main2() {
  // vector<BRay> beam;
  // beam.push_back(BRay(vec3(0, 0, 1), vec3(-1, 1, 1)));
  // beam.push_back(BRay(vec3(0, 0, 1), vec3(1, 1, 1)));
  // beam.push_back(BRay(vec3(0, 0, 1), vec3(1, 0, 1)));
  // beam.push_back(BRay(vec3(0, 0, 1), vec3(-1, 0, 1)));

  vector<vec3> tempv = {
      vec3(-1, 1, 1) + vec3(0, 0, 5),   vec3(1, 1, 1) + vec3(0, 0, 5),
      vec3(-1, 1, -1) + vec3(0, 0, 5),  vec3(1, 1, -1) + vec3(0, 0, 5),
      vec3(-1, -1, 1) + vec3(0, 0, 5),  vec3(1, -1, 1) + vec3(0, 0, 5),
      vec3(-1, -1, -1) + vec3(0, 0, 5), vec3(1, -1, -1) + vec3(0, 0, 5)};
  int tempi[36] = {0, 1, 2, 1, 3, 2, 6, 5, 4, 6, 7, 5, 0, 5, 1, 0, 4, 5,
                   2, 3, 6, 3, 7, 6, 0, 2, 4, 2, 6, 4, 1, 5, 3, 3, 5, 7};
  for (int i = 0; i < 12; i++) {
    cout << "polygon clipping" << endl;
    // polygonClipping(beam, vector<vec3>{tempv[2], tempv[3],
    // tempv[6]});
    // polygonClipping(beam, vector<vec3>{tempv[tempi[3 * i]],
    //                                         tempv[tempi[3 * i + 1]],
    //                                         tempv[tempi[3 * i + 2]]});
  }
  return 0;
  vector<vec2> subjectData = {vec2(1, 3), vec2(1, 7), vec2(7, 7), vec2(7, 3)};
  vector<vec2> clipData = {vec2(2, 1), vec2(2, 5), vec2(4, 2), vec2(6.5f, 4.5f),
                           vec2(6.5f, 1.5f)};
  // reverse(clipData.begin(), clipData.end());
  vertex *subject = buildPolygon(subjectData);
  vertex *clip = buildPolygon(clipData);
  vertex *subjectHead = subject;
  vertex *clipHead = clip;
  cout << "First subject " << subject << " " << subject->toString() << endl;

  cout << "In polygon check " << pointInPolygon(clip, vec2(3, 3)) << endl;

  for (int i = 0; i < 10; i++) {
    cout << "0 subject " << subject << " " << subject->toString() << endl;
    subject = subject->next;
  }
  for (int i = 0; i < 10; i++) {
    cout << "0 clip " << clip << " " << clip->toString() << endl;
    clip = clip->next;
  }

  GHphase1(subject, clip);

  for (int i = 0; i < 10; i++) {
    cout << "1 subject " << subject << " " << subject->toString() << endl;
    subject = subject->next;
  }
  for (int i = 0; i < 10; i++) {
    cout << "1 clip " << clip << " " << clip->toString() << endl;
    clip = clip->next;
  }
  subject = subjectHead;
  clip = clipHead;
  GHphase2(subject, clip);
  for (int i = 0; i < 10; i++) {
    cout << "2 subject " << subject << " " << subject->toString() << " "
         << subject->entry << endl;
    subject = subject->next;
  }
  for (int i = 0; i < 10; i++) {
    cout << "2 clip " << clip << " " << clip->toString() << " " << clip->entry
         << endl;
    clip = clip->next;
  }
  vertex *result = GHphase3(subject, clip);
  vertex *resultHead = result;
  for (int i = 0; i < 10; i++) {

    cout << "3 result " << result << " " << result->toString() << " "
         << (result == nullptr) << endl;
    result = result->next;
    if (result == resultHead) {
      if (result->nextPoly != nullptr) {
        result = result->nextPoly;
        resultHead = result;
        cout << endl;
      } else {
        break;
      }
    }
  }
  return 0;
}
