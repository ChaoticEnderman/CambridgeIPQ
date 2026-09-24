#include <SDL2/SDL.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <glm/glm.hpp>

#include "camera.h"
#include "color.h"

using namespace glm;

int r, g, b, a;
SDL_Renderer *renderer;

void setRenderer(SDL_Renderer *r) { renderer = r; }
SDL_Renderer *getRenderer() { return renderer; }

void nextFrame() { SDL_RenderPresent(renderer); }

void clear() {
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);
}

void setColor(color color) {
  SDL_SetRenderDrawColor(renderer, color.r, color.b, color.g, color.a);
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
