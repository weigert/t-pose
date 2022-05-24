#version 460 core

in vec3 in_Position;
in ivec4 in_Index;

layout (std430, binding = 0) buffer points {
  vec2 p[];
};

layout (std430, binding = 1) buffer index {
  ivec4 ind[];
};

layout (std430, binding = 3) buffer otherpoints {
  vec2 op[];
};

out VS_OUT {

  vec2 position;
  vec2 opos;
  flat int index;

} vs_out;

uniform float RATIO;
uniform int KTriangles;
float s = 0;

void main() {

  vec2 tpos = vec2(0);
  vec2 opos = vec2(0);
  float dp = 0.01f;
  int TDIV = gl_InstanceID/KTriangles;
  int TMOD = gl_InstanceID%KTriangles;
  vs_out.index = gl_InstanceID;

  if (in_Position.x > 0) tpos = mix(p[ind[TMOD].x], op[ind[TMOD].x], s);
  if (in_Position.y > 0) tpos = mix(p[ind[TMOD].y], op[ind[TMOD].y], s);
  if (in_Position.z > 0) tpos = mix(p[ind[TMOD].z], op[ind[TMOD].z], s);
  tpos.x /= RATIO;

  if (in_Position.x > 0) opos = mix(p[ind[TMOD].x], op[ind[TMOD].x], 1);
  if (in_Position.y > 0) opos = mix(p[ind[TMOD].y], op[ind[TMOD].y], 1);
  if (in_Position.z > 0) opos = mix(p[ind[TMOD].z], op[ind[TMOD].z], 1);
  opos.x /= RATIO;

  gl_Position = vec4(tpos, -1, 1.0f);
  vs_out.position = vec2(0.5*(1.0+tpos.x), 0.5*(1.0-tpos.y));
  vs_out.opos = vec2(0.5*(1.0+opos.x), 0.5*(1.0-opos.y));

}
