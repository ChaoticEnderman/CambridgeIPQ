#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <glm/glm.hpp>

void setRenderer(SDL_Renderer *r);
SDL_Renderer *getRenderer();

void nextFrame();

void setColor(int r, int g, int b, int a);
void setColor(int r, int g, int b);

void drawPoint(int x, int y);
void drawLine(int x1, int y1, int x2, int y2);
