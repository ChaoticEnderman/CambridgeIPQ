#include <algorithm>
#include <glm/glm.hpp>

#include "color.h"

using namespace glm;

constexpr double PI = 3.14159265358979323846;

float Fs_D_GGX(float NoH, float roughness) {
  // calculate D component in specular reflection
  // no need to optimize as per the paper since cpu float accuracy is 32 bit
  float a = NoH * roughness;
  float k = roughness / (1.0 - NoH * NoH + a * a);
  return k * k * (1.0 / PI);
}

float Fs_V_SmithGGXCorrelatedFast(float NoV, float NoL, float roughness) {
  // calculate G component in specular reflection
  // use V to subtitute to simplify the G component as in the paper
  // fast algorithm that ignore square root since the terms under square root is
  // in 0-1 range
  float a = roughness;
  float GGXV = NoL * (NoV * (1.0 - a) + a);
  float GGXL = NoV * (NoL * (1.0 - a) + a);
  return 0.5 / (GGXV + GGXL);
}

vec3 Fs_F_Schlick(float u, vec3 f0) {
  // calculate F component in specular reflection
  // Fresnel effect
  // Schlick approximation for real-time code
  // assume f_90 = 1 for faster math in the scope of this test
  float f = pow(1.0 - u, 5.0);
  return f + f0 * (1.0f - f);
}

float Fd_Lambert() { return 1.0 / PI; }

void BRDF(vec3 v, vec3 l, vec3 n, float r, vec3 f0, color albedo) {
  vec3 h = normalize(v + l);

  float NoV = abs(dot(n, v)) + 1e-5;
  float NoL = clamp(dot(n, l), 0.0f, 1.0f);
  float NoH = clamp(dot(n, h), 0.0f, 1.0f);
  float LoH = clamp(dot(l, h), 0.0f, 1.0f);

  // change linear roughness to roughness
  float roughness = r * r;

  float Fs_D = Fs_D_GGX(NoH, roughness);
  vec3 Fs_F = Fs_F_Schlick(LoH, f0);
  float Fs_V = Fs_V_SmithGGXCorrelatedFast(NoV, NoL, roughness);

  // specular component
  vec3 Fs = (Fs_D * Fs_V) * Fs_F;

  // diffuse component
  vec3 Fd = albedo.scaleColor(Fd_Lambert()).toVec3();

  // apply lighting...
}