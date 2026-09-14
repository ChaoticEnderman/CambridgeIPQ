#include <SDL2/SDL.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <glm/glm.hpp>

#include "camera.h"

int r, g, b, a;
SDL_Renderer *renderer;

void setRenderer(SDL_Renderer *r) { renderer = r; }
SDL_Renderer *getRenderer() { return renderer; }

void nextFrame() { SDL_RenderPresent(renderer); }

void clear() {
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);
}

void setColor(int r, int g, int b, int a) {
  SDL_SetRenderDrawColor(renderer, r, g, b, a);
}
void setColor(int r, int g, int b) {
  SDL_SetRenderDrawColor(renderer, r, g, b, 255);
}

void drawPoint(int x, int y) { SDL_RenderDrawPoint(renderer, x, y); }

void drawLine(int x1, int y1, int x2, int y2) {
  SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
}
