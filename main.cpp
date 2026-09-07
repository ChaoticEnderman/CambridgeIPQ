#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_timer.h>
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
float delta;
bool running;

// Code that execute once
void initSDL() {
  srand(time(0));
  glm::ivec2 vertices[3] = {glm::ivec2(0, 0), glm::ivec2(100, 50),
                            glm::ivec2(50, 200)};
  for (int i = 0; i < 100; i++) {
    vertices[0] = glm::ivec2(rand() % getScreenResolution().x,
                             rand() % getScreenResolution().y);
    vertices[1] = glm::ivec2(rand() % getScreenResolution().x,
                             rand() % getScreenResolution().y);
    vertices[1] = glm::ivec2(rand() % getScreenResolution().x,
                             rand() % getScreenResolution().y);
    raster2DTriangle(vertices, rand() % 255, rand() % 255, rand() % 255);
  }
}
// Code that execute every frame
void loopSDL() {}

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
      case SDL_KEYDOWN:
        if (!e.key.repeat) {
          switch (e.key.keysym.sym) {
          case SDLK_ESCAPE:
            running = 0;
            break;
          case SDLK_w:
            test++;
            std::cout << "W pressed " << test << std::endl;
            break;
          }
        }
      }
    }
    currentTick = SDL_GetTicks();
    delta = (currentTick - previousTick) / 1000.0f;
    std::cout << "delta " << delta << std::endl;
    loopSDL();

    SDL_RenderPresent(getRenderer());
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
