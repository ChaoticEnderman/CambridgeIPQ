#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_timer.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <glm/detail/qualifier.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_int2.hpp>
#include <iterator>
#include <stdlib.h>

#include <chrono>
#include <cstdlib>
#include <glm/glm.hpp>
#include <iostream>
#include <vector>

#include "beamtracer.h"
#include "camera.h"
#include "color.h"
#include "mesh.h"
#include "rasterizer.h"
#include "raytracer.h"
#include "renderer.h"

using namespace glm;
constexpr double pi = 3.14159265358979323846;

int newPolygonTest = 0;

std::vector<vec3> subject = {};
std::vector<vec3> clip = {};
std::vector<vec2> result = {};

SDL_Window *window;
Uint32 previousTick;
Uint32 currentTick;
const Uint8 *keystate = SDL_GetKeyboardState(NULL);
float delta;
bool running;
float accFrameDelta = 0.0f;
int accFrames = 0;

vec3 movement;
vec3 rotation;

float radius;
vec3 pos;
vec3 vertices[8];
int indices[36];
color colors[36];

mesh prepareCubeMesh(vec3 pos, color c) {
  radius = 1.0f;
  vec3 tempv[8] = {vec3(-radius, radius, radius) + pos,
                   vec3(radius, radius, radius) + pos,
                   vec3(-radius, radius, -radius) + pos,
                   vec3(radius, radius, -radius) + pos,
                   vec3(-radius, -radius, radius) + pos,
                   vec3(radius, -radius, radius) + pos,
                   vec3(-radius, -radius, -radius) + pos,
                   vec3(radius, -radius, -radius) + pos};
  memcpy(vertices, tempv, sizeof(vertices));
  int tempi[36] = {// top face
                   0, 1, 2, 1, 3, 2,
                   // bottom face
                   6, 5, 4, 6, 7, 5,
                   // back face
                   0, 5, 1, 0, 4, 5,
                   // front face
                   2, 3, 6, 3, 7, 6,
                   // left face
                   0, 2, 4, 2, 6, 4,
                   // right face
                   1, 5, 3, 3, 5, 7};
  memcpy(indices, tempi, sizeof(indices));
  color tempc[36];
  std::fill(tempc, tempc + 36, c);
  memcpy(colors, tempc, sizeof(colors));
  std::vector<vec3> vecVertices3d(std::begin(vertices), std::end(vertices));
  std::vector<int> vecIndices(std::begin(indices), std::end(indices));
  std::vector<color> vecColors(std::begin(colors), std::end(colors));
  mesh m = mesh(vecVertices3d, std::vector<vec3>(), vecColors, vecIndices,
                material::base::redFill);
  addMesh(m);
  return m;
}

// Code that execute once
void initSDL() {
  srand(time(0));
  setDefaultTexture();
  mesh m1 = prepareCubeMesh(vec3(0.0f, 0.0f, 4.0f), color::base::red);
  // mesh m2 = prepareCubeMesh(vec3(2.0f, 0.0f, 4.0f), color::base::green);
  //  traceRays();
}

void drawIntersection() {
  setColor(color::base::red);
  for (int i = 0; i < subject.size(); i++) {
    if (i != 0) {
      drawLine(subject[i - 1].x, subject[i - 1].y, subject[i].x, subject[i].y);
    }
    if (i == subject.size() - 1) {
      drawLine(subject[i].x, subject[i].y, subject[0].x, subject[0].y);
    }
  }
  for (int i = 0; i < clip.size(); i++) {
    if (i != 0) {
      drawLine(clip[i - 1].x, clip[i - 1].y, clip[i].x, clip[i].y);
    }
    if (i == clip.size() - 1) {
      drawLine(clip[i].x, clip[i].y, clip[0].x, clip[0].y);
    }
  }
  setColor(color::base::green);
  int head = 0;
  std::cout << "intersection size " << subject.size() << " " << clip.size()
            << std::endl;
  result = testClipping(subject, clip);
  std::vector<vec2> subres = {};
  std::cout << "intersection count " << result.size() << std::endl;
  for (int i = 0; i < result.size(); i++) {
    if (result[i].x < 1000000) {
      subres.push_back(result[i]);
    } else {
      for (int j = 0; j < subres.size(); j++) {
        if (j < subres.size() - 1) {
          drawLine(subres[j + 1].x, subres[j + 1].y, subres[j].x, subres[j].y);
        } else {
          drawLine(subres[j].x, subres[j].y, subres[0].x, subres[0].y);
        }
      }
      subres.clear();
    }
  };
}

void randomizePolygon() {
  int size = std::rand() % 5 + 3;
  // size = subject.size();
  setColor(color::base::red);
  for (int i = 0; i < size; i++) {
    int x = std::rand() % getScreenResolutionX();
    int y = std::rand() % getScreenResolutionY();
    subject.push_back(vec3(x, y, 0));
  }
  setColor(color::base::blue);
  size = std::rand() % 5 + 3;
  // size = clip.size();
  for (int i = 0; i < size; i++) {
    int x = std::rand() % getScreenResolutionX();
    int y = std::rand() % getScreenResolutionY();
    clip.push_back(vec3(x, y, 0));
  }
}

void setPolygon1() {
  subject = {vec3(25, 150, 0), vec3(50, 375, 0), vec3(350, 350, 0),
             vec3(350, 150, 0)};
  clip = {vec3(25, 50, 0), vec3(100, 250, 0), vec3(200, 100, 0),
          vec3(300, 225, 0), vec3(325, 75, 0)};
}

void setPolygon2() {
  subject = {vec3(100, 150, 0), vec3(100, 300, 0), vec3(300, 300, 0),
             vec3(300, 150, 0)};
  clip = {vec3(200, 150, 0), vec3(200, 300, 0), vec3(350, 300, 0),
          vec3(350, 150, 0)};
}

// 3. Vertex-on-vertex touch (single shared corner, no overlap area).
void setPolygon3() {
  subject = {vec3(100, 100, 0), vec3(100, 200, 0), vec3(200, 200, 0),
             vec3(200, 100, 0)};
  clip = {vec3(200, 200, 0), vec3(200, 300, 0), vec3(300, 300, 0),
          vec3(300, 200, 0)};
}

// 4. T-junction, clip vertex touches subject edge from outside.
// Clip apex at (200,300) lies on subject's top edge.
void setPolygon4() {
  subject = {vec3(100, 100, 0), vec3(100, 300, 0), vec3(300, 300, 0),
             vec3(300, 100, 0)};
  clip = {vec3(200, 300, 0), vec3(150, 380, 0), vec3(250, 380, 0)};
}

// 5. T-junction, clip vertex touches subject edge from inside.
// Clip apex at (200,300) lies on subject's top edge, rest of clip is inside.
void setPolygon5() {
  subject = {vec3(100, 100, 0), vec3(100, 300, 0), vec3(300, 300, 0),
             vec3(300, 100, 0)};
  clip = {vec3(200, 300, 0), vec3(150, 200, 0), vec3(250, 200, 0)};
}

// 6. T-junction reversed: a subject vertex lies on a clip edge.
void setPolygon6() {
  subject = {vec3(200, 200, 0), vec3(150, 120, 0), vec3(250, 120, 0)};
  clip = {vec3(100, 200, 0), vec3(100, 350, 0), vec3(300, 350, 0),
          vec3(300, 200, 0)};
}

// 7. Identical polygons (every vertex and edge coincides).
void setPolygon7() {
  subject = {vec3(100, 100, 0), vec3(100, 300, 0), vec3(300, 300, 0),
             vec3(300, 100, 0)};
  clip = subject;
}

// 8. Abutting polygons sharing a full edge (touch along a segment, zero area).
void setPolygon8() {
  subject = {vec3(100, 100, 0), vec3(100, 300, 0), vec3(200, 300, 0),
             vec3(200, 100, 0)};
  clip = {vec3(200, 100, 0), vec3(200, 300, 0), vec3(300, 300, 0),
          vec3(300, 100, 0)};
}

// 9. Abutting along a partial edge (clip edge is a sub-segment of subject edge;
// clip vertices are T-junctions on the subject edge).
void setPolygon9() {
  subject = {vec3(100, 100, 0), vec3(100, 300, 0), vec3(200, 300, 0),
             vec3(200, 100, 0)};
  clip = {vec3(200, 150, 0), vec3(200, 250, 0), vec3(300, 250, 0),
          vec3(300, 150, 0)};
}

// 10. Crossing through a vertex: clip diamond's vertex lands exactly on a
// subject corner and the diamond passes through (V-V, transversal).
void setPolygon10() {
  subject = {vec3(100, 100, 0), vec3(100, 250, 0), vec3(250, 250, 0),
             vec3(250, 100, 0)};
  clip = {vec3(250, 250, 0), vec3(200, 300, 0), vec3(150, 250, 0),
          vec3(200, 200, 0)};
}

// 11. Clip edge passes exactly through a subject vertex (edge-through-vertex).
// Clip's diagonal edge from (50,50) to (200,200) passes exactly through the
// subject corner (100,100), which is not a clip vertex.
void setPolygon11() {
  subject = {vec3(100, 100, 0), vec3(100, 250, 0), vec3(250, 250, 0),
             vec3(250, 100, 0)};
  clip = {vec3(50, 50, 0), vec3(200, 200, 0), vec3(50, 200, 0)};
}

// 12. Internal tangency: clip fully inside subject, sharing one vertex and
// touching along part of an edge.
void setPolygon12() {
  subject = {vec3(100, 100, 0), vec3(100, 300, 0), vec3(300, 300, 0),
             vec3(300, 100, 0)};
  clip = {vec3(100, 100, 0), vec3(100, 200, 0), vec3(200, 200, 0),
          vec3(200, 100, 0)};
}

// 13. Diamond inscribed in square: every clip vertex is a T-junction at an
// edge midpoint of subject (4 touch points, intersection = whole diamond).
void setPolygon13() {
  subject = {vec3(100, 100, 0), vec3(100, 300, 0), vec3(300, 300, 0),
             vec3(300, 100, 0)};
  clip = {vec3(200, 100, 0), vec3(300, 200, 0), vec3(200, 300, 0),
          vec3(100, 200, 0)};
}

// 14. Opposite winding: same overlapping-edge setup as case 2 but clip is
// reversed (CW), so collinear edges run in opposite directions.
void setPolygon14() {
  subject = {vec3(100, 150, 0), vec3(100, 300, 0), vec3(300, 300, 0),
             vec3(300, 150, 0)};
  clip = {vec3(350, 150, 0), vec3(350, 300, 0), vec3(200, 300, 0),
          vec3(200, 150, 0)};
}

// 15. Collinear edge with clip vertices outside the subject edge's extent
// (clip edge fully contains subject edge, so subject vertices are
// T-junctions on the clip edge).
void setPolygon15() {
  subject = {vec3(150, 100, 0), vec3(150, 250, 0), vec3(250, 250, 0),
             vec3(250, 100, 0)};
  clip = {vec3(100, 250, 0), vec3(100, 350, 0), vec3(300, 350, 0),
          vec3(300, 250, 0)};
}

// 16. Vertex touching vertex from inside a concave notch: clip tip lands
// exactly on the reflex vertex of a concave subject.
void setPolygon16() {
  subject = {vec3(100, 100, 0), vec3(100, 300, 0), vec3(200, 200, 0),
             vec3(300, 300, 0), vec3(300, 100, 0)};
  clip = {vec3(200, 200, 0), vec3(150, 120, 0), vec3(250, 120, 0)};
}

// 17. Two shared vertices with a diagonal crossing between them
// (clip shares subject's bottom-left and top-right corners).
void setPolygon17() {
  subject = {vec3(100, 100, 0), vec3(100, 300, 0), vec3(300, 300, 0),
             vec3(300, 100, 0)};
  clip = {vec3(100, 100, 0), vec3(200, 250, 0), vec3(300, 300, 0),
          vec3(250, 150, 0)};
}

// Code that execute every frames
void loopSDL() {
  // clear();

  computeRotationMatrixes();
  computeCameraMatrixes();
  clearZBuffer();
  std::cout << "new polygon here " << newPolygonTest << std::endl;
  if (newPolygonTest == 0) {
    clear();
    subject.clear();
    clip.clear();
    newPolygonTest = 1;
    // randomizePolygon();
    setPolygon1();
  } else if (newPolygonTest == 2) {
    drawIntersection();
  } else if (newPolygonTest == 3) {
    newPolygonTest = 0;
  }
  std::chrono::time_point start = std::chrono::steady_clock::now();
  // testSplitAnnulus();
  // traceBeams();
  // traceRays();
  // raster3Dcoord();
  // raster3DMesh(getMesh(0));
  // raster3DMesh(getMesh(1));
  std::chrono::time_point end = std::chrono::steady_clock::now();
  std::chrono::nanoseconds duration =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
  // std::cout << "Frame time: " << duration.count() << " ns\n";
  //  updateTexture();
  nextFrame(); // this is mandatory for some reason
}

// Some code for initializing
int init() {
  // Initialize video and quit when failed
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::cout << "Init failed: " << SDL_GetError() << std::endl;
    return 1;
  }

  // Create a window and quit when failed
  window = SDL_CreateWindow("SDL2 Test", SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, getScreenResolution().x,
                            getScreenResolution().y, SDL_WINDOW_SHOWN);
  if (!window) {
    std::cout << "Window failed: " << SDL_GetError() << std::endl;
    SDL_Quit();
    return 1;
  }

  // Create a renderer for the window and quit when failed
  setRenderer(SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE));
  if (!getRenderer()) {
    std::cout << "Renderer failed: " << SDL_GetError() << std::endl;
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  previousTick = SDL_GetTicks();
  running = true;
  initSDL();
  int test = 0;
  while (running) {
    SDL_Event e;
    // poll for input
    while (SDL_PollEvent(&e)) {
      movement = vec3(0.0f, 0.0f, 0.0f);
      rotation = vec3(0.0f, 0.0f, 0.0f);
      // polling for quit signal
      if (e.type == SDL_QUIT) {
        running = false;
        break;
      }
      if (keystate[SDL_SCANCODE_W]) {
        movement.z += 1.0f;
        std::cout << "w" << std::endl;
      }
      if (keystate[SDL_SCANCODE_S]) {
        movement.z -= 1.0f;
      }
      if (keystate[SDL_SCANCODE_D]) {
        movement.x += 1.0f;
      }
      if (keystate[SDL_SCANCODE_A]) {
        movement.x -= 1.0f;
      }
      if (keystate[SDL_SCANCODE_E]) {
        movement.y += 1.0f;
      }
      if (keystate[SDL_SCANCODE_Q]) {
        movement.y -= 1.0f;
      }
      if (keystate[SDL_SCANCODE_UP]) {
        rotation.x -= 1.0f;
      }
      if (keystate[SDL_SCANCODE_DOWN]) {
        rotation.x += 1.0f;
      }
      if (keystate[SDL_SCANCODE_RIGHT]) {
        rotation.y += 1.0f;
      }
      if (keystate[SDL_SCANCODE_LEFT]) {
        rotation.y -= 1.0f;
      }
      if (e.type == SDL_KEYDOWN && e.key.repeat == 0) {
        if (e.key.keysym.sym == SDLK_TAB) {
          newPolygonTest += 1;
          // std::cout << "new polygon test " << newPolygonTest << std::endl;
        }
      }
    }

    // camera movement
    setCameraRotation(getCameraRotation() + rotation * delta * 5.0f);
    setCameraRotation(
        vec3(std::clamp(getCameraRotation().x, (float)-pi / 2, (float)pi / 2),
             getCameraRotation().y, 0.0f));
    float rotY = -getCameraRotation().y;
    mat2x2 rotationMatrix = mat2x2(cos(rotY), sin(rotY), -sin(rotY), cos(rotY));
    vec2 move2 = vec2(movement.x, movement.z);
    move2 = rotationMatrix * move2;
    setCameraPosition(getCameraPosition() +
                      vec3(move2.x, movement.y, move2.y) * delta * 10.0f);
    // std::cout << "movement " << move2.x << " " << movement.y << " " <<
    // move2.y
    //           << std::endl;
    currentTick = SDL_GetTicks();

    // profiling
    if (false) {
      std::cout << "delta " << delta << " cam position "
                << getCameraPosition().x << " " << getCameraPosition().y << " "
                << getCameraPosition().z << " cam rotation "
                << getCameraRotation().x << " " << getCameraRotation().y << " "
                << getCameraPosition().z << std::endl;
    }
    loopSDL();

    delta = (currentTick - previousTick) / 1000.0f;
    accFrameDelta += delta;
    accFrames += 1;
    if (accFrameDelta > 1.0f) {
      std::cout << "fps this second " << accFrames << std::endl;
      accFrameDelta -= 1.0f;
      accFrames = 0;
    }

    previousTick = currentTick;
  }

  return 0;
}

void quitSDL() {
  SDL_DestroyRenderer(getRenderer());
  SDL_DestroyWindow(window);
  SDL_Quit();
}

int main() {
  init();
  return 0;
}
