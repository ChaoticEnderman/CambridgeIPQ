#include <SDL2/SDL.h>
#include <stdlib.h>

#include <glm/glm.hpp>
#include <iostream>

#include "camera.h"
#include "rasterizer.h"
#include "renderer.h"

SDL_Window* window;
SDL_Renderer* renderer;
Uint32 start;
bool running;
// put code here to execute
void loopSDL() {
  glm::ivec2 vertices[3];
  vertices[0] = glm::ivec2(0, 0);
  vertices[1] = glm::ivec2(100, 50);
  vertices[2] = glm::ivec2(50, 200);
  raster2DTriangle(vertices);
  std::cout << "looping end" << std::endl;
  setColor(0, 255, 255);
  drawLine(0, 0, 400, 400);
}

glm::ivec2 getScreenResolution() { return glm::ivec2{640, 360}; }

// The code below in this file isnt the main focus for the rest of the project
int initSDL() {
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
  renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_SOFTWARE);  // explicitly use software renderer
  if (!renderer) {
    std::cout << "Renderer failed: " << SDL_GetError() << std::endl;
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  start = SDL_GetTicks();
  running = true;
  setRenderer(renderer);
  while (running) {
    SDL_Event e;
    // poll for input
    while (SDL_PollEvent(&e)) {
      // polling for quit signal
      if (e.type == SDL_QUIT) {
        running = false;
      }
    }

    loopSDL();

    SDL_RenderPresent(renderer);
    // 16ms for 60fps
    SDL_Delay(16);
  }

  return 0;
}

void quitSDL() {
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

int main() {
  initSDL();
  return 0;
}
