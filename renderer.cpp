#include <SDL2/SDL.h>

int r, g, b, a;
SDL_Renderer* renderer;

void setRenderer(SDL_Renderer* r) { renderer = r; }

void nextFrame() { SDL_RenderPresent(renderer); }

void setColor(int r, int b, int g, int a) {
  SDL_SetRenderDrawColor(renderer, r, b, g, a);
}
void setColor(int r, int b, int g) {
  SDL_SetRenderDrawColor(renderer, r, b, g, 255);
}

void drawPoint(int x, int y) { SDL_RenderDrawPoint(renderer, x, y); }

void drawLine(int x1, int y1, int x2, int y2) {
  SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
}
