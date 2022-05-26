#version 460 core

in vec3 in_Position;

layout (std430, binding = 1) buffer index {
  ivec4 ind[];
};

layout (std430, binding = 3) buffer points2D {
  vec2 p[];
};

layout (std430, binding = 4) buffer dist2D {
  float dist[];
};

uniform float RATIO;
out float d;

void main() {

  vec2 tpos = vec2(0);
  int pind;

  if (in_Position.x > 0) pind = ind[gl_InstanceID].x;
  if (in_Position.y > 0) pind = ind[gl_InstanceID].y;
  if (in_Position.z > 0) pind = ind[gl_InstanceID].z;

  tpos = p[pind];
  d = dist[pind];
  tpos.x /= RATIO;

  gl_Position = vec4(tpos, -1, 1.0f);

}
