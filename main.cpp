#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_timer.h>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_int2.hpp>
#include <stdlib.h>

#include <glm/glm.hpp>
#include <iostream>

#include "camera.h"
#include "mesh.h"
#include "rasterizer.h"
#include "renderer.h"
#include <cstdlib>

SDL_Window *window;
Uint32 previousTick;
Uint32 currentTick;
const Uint8 *keystate = SDL_GetKeyboardState(NULL);
float delta;
bool running;

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
// Code that execute every frames
void loopSDL() {
  clear();
  computeRotationMatrixes();
  glm::vec3 vertices3d[12][3] = {
      {glm::vec3(1.0f, -1.0f, 3.0f), glm::vec3(1.0f, 1.0f, 3.0f),
       glm::vec3(1.0f, 1.0f, 5.0f)},
      {glm::vec3(1.0f, 1.0f, 3.0f), glm::vec3(1.0f, 1.0f, 5.0f),
       glm::vec3(-1.0f, 1.0f, 3.0f)},
      {glm::vec3(-1.0f, 1.0f, 3.0f), glm::vec3(-1.0f, -1.0f, 3.0f),
       glm::vec3(1.0f, 1.0f, 3.0f)},
      {glm::vec3(-1.0f, -1.0f, 3.0f), glm::vec3(1.0f, -1.0f, 3.0f),
       glm::vec3(1.0f, 1.0f, 3.0f)},
  };
  raster3DTriangle(vertices3d[0], 255, 0, 0);
  raster3DTriangle(vertices3d[1], 200, 0, 0);
  raster3DTriangle(vertices3d[2], 0, 255, 0);
  raster3DTriangle(vertices3d[3], 0, 200, 0);

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
      rotation.x += 1.0f;
    }
    if (keystate[SDL_SCANCODE_DOWN]) {
      rotation.x -= 1.0f;
    }
    if (keystate[SDL_SCANCODE_RIGHT]) {
      rotation.y += 1.0f;
    }
    if (keystate[SDL_SCANCODE_LEFT]) {
      rotation.y -= 1.0f;
    }
    setCameraPosition(getCameraPosition() + movement * 0.05f);
    setCameraRotation(getCameraRotation() + rotation / 57.0f);
    currentTick = SDL_GetTicks();
    delta = (currentTick - previousTick) / 1000.0f;
    std::cout << "delta " << delta << " cam position " << getCameraPosition().x
              << " " << getCameraPosition().y << " " << getCameraPosition().z
              << " cam rotation " << getCameraRotation().x << " "
              << getCameraRotation().y << " " << getCameraPosition().z
              << std::endl;
    loopSDL();

    SDL_Delay(16);
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
