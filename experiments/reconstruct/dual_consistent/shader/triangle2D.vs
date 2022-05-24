#version 460 core

in vec3 in_Position;

layout (std430, binding = 0) buffer points {
  vec2 p[];
};

layout (std430, binding = 1) buffer index {
  ivec4 ind[];
};

uniform float RATIO;

void main() {

  vec2 tpos = vec2(0);
  if (in_Position.x > 0) tpos = p[ind[gl_InstanceID].x];
  if (in_Position.y > 0) tpos = p[ind[gl_InstanceID].y];
  if (in_Position.z > 0) tpos = p[ind[gl_InstanceID].z];
  tpos.x /= RATIO;

  gl_Position = vec4(tpos, -1, 1.0f);

}
