#version 460 core

in vec3 in_Position;

layout (std430, binding = 0) buffer index {
  ivec4 ind[];
};

layout (std430, binding = 2) buffer points2Dfrom {
  vec2 p0[];
};

layout (std430, binding = 3) buffer points2Dto {
  vec2 p1[];
};

uniform float RATIO;
uniform float warp;

flat out int ex_Index;

void main() {

  vec2 tpos = vec2(0);
  int pind;

  if (in_Position.x > 0) pind = ind[gl_InstanceID].x;
  if (in_Position.y > 0) pind = ind[gl_InstanceID].y;
  if (in_Position.z > 0) pind = ind[gl_InstanceID].z;

  tpos = mix(p0[pind], p1[pind], warp);
  tpos.x /= RATIO;
  ex_Index = gl_InstanceID;

  gl_Position = vec4(tpos, -1, 1.0f);

}
