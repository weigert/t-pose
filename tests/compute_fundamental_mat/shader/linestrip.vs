#version 460 core

in vec3 in_Position;
in ivec4 in_Index;

layout (std430, binding = 0) buffer points {
  vec4 p[];
};

layout (std430, binding = 1) buffer index {
  ivec4 ind[];
};

uniform float RATIO;
uniform float s;
uniform mat4 vp;
uniform mat4 model;

void main() {

  vec4 tpos = vec4(0);
  if (in_Position.x > 0) tpos = p[ind[gl_InstanceID].x];
  if (in_Position.y > 0) tpos = p[ind[gl_InstanceID].y];
  if (in_Position.z > 0) tpos = p[ind[gl_InstanceID].z];
  tpos.x /= RATIO;
  gl_Position = vp*model*tpos;

}
