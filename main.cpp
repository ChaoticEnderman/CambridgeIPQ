#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_timer.h>
#include <algorithm>
#include <glm/detail/qualifier.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_int2.hpp>
#include <iterator>
#include <stdlib.h>

#include <chrono>
#include <glm/glm.hpp>
#include <iostream>

#include "camera.h"
#include "rasterizer.h"
#include <cstdlib>
#include <vector>

constexpr double pi = 3.14159265358979323846;

SDL_Window *window;
Uint32 previousTick;
Uint32 currentTick;
const Uint8 *keystate = SDL_GetKeyboardState(NULL);
float delta;
bool running;
float accFrameDelta = 0.0f;
int accFrames = 0;

glm::vec3 movement;
glm::vec3 rotation;

// Code that execute once
void initSDL() {
  srand(time(0));
  // glm::ivec2 vertices[3] = {glm::ivec2(0, 0), glm::ivec2(100, 50),
  //                           glm::ivec2(50, 200)};
  // for (int i = 0; i < 100; i++) {
  //   vertices[0] = glm::ivec2(rand() % getScreenResolution().x,
  //                            rand() % getScreenResolution().y);
  //   vertices[1] = glm::ivec2(rand() % getScreenResolution().x,
  //                            rand() % getScreenResolution().y);
  //   vertices[1] = glm::ivec2(rand() % getScreenResolution().x,
  //                            rand() % getScreenResolution().y);
  //   raster2DTriangle(vertices, rand() % 255, rand() % 255, rand() % 255);
  // }
}

float radius = 1.0f;
glm::vec3 pos = glm::vec3(0.0f, 0.0f, 4.0f);
glm::vec3 vertices3d[8] = {glm::vec3(-radius, radius, radius) + pos,
                           glm::vec3(radius, radius, radius) + pos,
                           glm::vec3(-radius, radius, -radius) + pos,
                           glm::vec3(radius, radius, -radius) + pos,
                           glm::vec3(-radius, -radius, radius) + pos,
                           glm::vec3(radius, -radius, radius) + pos,
                           glm::vec3(-radius, -radius, -radius) + pos,
                           glm::vec3(radius, -radius, -radius) + pos};
int indices[36] = { // top face
    0, 1, 2, 1, 3, 2,
    // bottom face
    6, 5, 4, 6, 7, 5,
    // front face
    0, 5, 1, 0, 4, 5,
    // back face
    2, 3, 6, 3, 7, 6,
    // left face
    0, 2, 4, 2, 6, 4,
    // right face
    1, 5, 3, 3, 5, 7};
// Code that execute every frames
void loopSDL() {
  clear();

  computeRotationMatrixes();
  // glm::vec3 vertices3d[12][3] = {
  //     {glm::vec3(1.0f, -1.0f, 3.0f), glm::vec3(1.0f, 1.0f, 3.0f),
  //      glm::vec3(1.0f, 1.0f, 5.0f)},
  //     {glm::vec3(1.0f, -1.0f, 3.0f), glm::vec3(1.0f, -1.0f, 5.0f),
  //      glm::vec3(1.0f, 1.0f, 5.0f)},
  //     {glm::vec3(-1.0f, 1.0f, 3.0f), glm::vec3(-1.0f, -1.0f, 3.0f),
  //      glm::vec3(1.0f, 1.0f, 3.0f)},
  //     {glm::vec3(-1.0f, -1.0f, 3.0f), glm::vec3(1.0f, -1.0f, 3.0f),
  //      glm::vec3(1.0f, 1.0f, 3.0f)},
  // };
  // raster3DTriangle(vertices3d[0], 255, 0, 0);
  // raster3DTriangle(vertices3d[1], 200, 0, 0);
  // raster3DTriangle(vertices3d[2], 0, 255, 0);
  // raster3DTriangle(vertices3d[3], 0, 200, 0);
  std::vector<glm::vec3> vecVertices3d(std::begin(vertices3d),
                                       std::end(vertices3d));
  std::vector<int> vecIndices(std::begin(indices), std::end(indices));
  mesh m = mesh(vecVertices3d, std::vector<glm::vec3>(), vecIndices);
  auto start = std::chrono::steady_clock::now();
  raster3DMesh(m);
  auto end = std::chrono::steady_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  std::cout << "Elapsed: " << duration.count() << " us\n";
  nextFrame();
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
      // polling for quit signal
      switch (e.type) {
      case SDL_QUIT:
        running = false;
        break;
      }
    }
    movement = glm::vec3(0.0f, 0.0f, 0.0f);
    rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    if (keystate[SDL_SCANCODE_W]) {
      movement.z += 1.0f;
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
    // camera movement
    setCameraRotation(getCameraRotation() + rotation / 20.0f);
    setCameraRotation(glm::vec3(
        std::clamp(getCameraRotation().x, (float)-pi / 2, (float)pi / 2),
        getCameraRotation().y, 0.0f));
    float rotY = -getCameraRotation().y;
    glm::mat2x2 rotationMatrix =
        glm::mat2x2(cos(rotY), sin(rotY), -sin(rotY), cos(rotY));
    glm::vec2 move2 = glm::vec2(movement.x, movement.z);
    move2 = rotationMatrix * move2;
    setCameraPosition(getCameraPosition() +
                      glm::vec3(move2.x, movement.y, move2.y) * 0.10f);
    currentTick = SDL_GetTicks();
    delta = (currentTick - previousTick) / 1000.0f;
    accFrameDelta += delta;
    accFrames += 1;
    if (accFrameDelta > 1.0f) {
      std::cout << "fps this second " << accFrames << std::endl;
      accFrameDelta -= 1.0f;
      accFrames = 0;
    }

    // profiling
    std::cout << "delta " << delta << " cam position " << getCameraPosition().x
              << " " << getCameraPosition().y << " " << getCameraPosition().z
              << " cam rotation " << getCameraRotation().x << " "
              << getCameraRotation().y << " " << getCameraPosition().z
              << std::endl;
    loopSDL();

    // SDL_Delay(16);
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
