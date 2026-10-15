#include <SDL2/SDL.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <iostream>
#include <string>

#include "camera.h"
#include "color.h"

using namespace glm;

int r, g, b, a;
SDL_Renderer *renderer;
SDL_Texture *texture;
uint32_t fragment[getScreenResolutionX() * getScreenResolutionY()];

void setRenderer(SDL_Renderer *r) { renderer = r; }
SDL_Renderer *getRenderer() { return renderer; }

void setDefaultTexture() {
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                              SDL_TEXTUREACCESS_STREAMING,
                              getScreenResolutionX(), getScreenResolutionY());
}

void updateTexture() {
  SDL_UpdateTexture(texture, NULL, fragment,
                    getScreenResolutionX() * sizeof(uint32_t));
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  memset(fragment, 0,
         getScreenResolutionX() * getScreenResolutionY() * sizeof(uint32_t));
}

void nextFrame() { SDL_RenderPresent(renderer); }

void clear() {
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);
}

void setColor(color color) {
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

void drawPoint(int x, int y) { SDL_RenderDrawPoint(renderer, x, y); }
void drawPoints(ivec2 points[], int size) {
  SDL_Point p[size];
  for (int i = 0; i < size; i++) {
    p[i] = {points[i].x, points[i].y};
  }
  SDL_RenderDrawPoints(renderer, p, size);
}

void drawLine(int x1, int y1, int x2, int y2) {
  SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
}