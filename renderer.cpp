// Class for abstracting SDL2's draw calls, using functionalities that will be
// needed in this project
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
