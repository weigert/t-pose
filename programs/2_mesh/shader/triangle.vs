#version 460 core

in vec3 in_Position;
in ivec4 in_Index;

layout (std430, binding = 0) buffer points {
  vec4 p[];
};

layout (std430, binding = 1) buffer index {
  ivec4 ind[];
};

out VS_OUT {

  flat int index;

} vs_out;

uniform float RATIO;
uniform int KTriangles;
uniform float s;
uniform mat4 vp;
uniform mat4 model;

void main() {

  vec4 tpos = vec4(0);
  float dp = 0.01f;
  int TDIV = gl_InstanceID/KTriangles;
  int TMOD = gl_InstanceID%KTriangles;
  vs_out.index = gl_InstanceID;

  if (in_Position.x > 0) tpos = p[ind[TMOD].x];
  if (in_Position.y > 0) tpos = p[ind[TMOD].y];
  if (in_Position.z > 0) tpos = p[ind[TMOD].z];
  tpos.x /= RATIO;

  gl_Position = vp*model*tpos;

}
