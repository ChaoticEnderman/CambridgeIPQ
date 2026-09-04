#include <SDL2/SDL.h>

void setRenderer(SDL_Renderer* r);

void nextFrame();

void setColor(int r, int b, int g, int a);
void setColor(int r, int b, int g);

void drawPoint(int x, int y);
void drawLine(int x1, int y1, int x2, int y2);
