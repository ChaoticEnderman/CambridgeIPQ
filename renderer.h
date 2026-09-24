#pragma once
#include "color.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <glm/glm.hpp>

using namespace glm;

void setRenderer(SDL_Renderer *r);
SDL_Renderer *getRenderer();

void nextFrame();
void clear();

void setColor(color color);

void drawPoint(int x, int y);
void drawPoints(ivec2 points[], int size);
void drawLine(int x1, int y1, int x2, int y2);
