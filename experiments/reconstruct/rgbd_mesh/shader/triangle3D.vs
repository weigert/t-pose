#version 460 core

in vec3 in_Position;
in ivec4 in_Index;

layout (std430, binding = 3) buffer points3D {
  vec4 p[];
};

layout (std430, binding = 4) buffer otherindex {
  ivec4 ind[];
};

layout (std430, binding = 6) buffer points2D {
  vec2 o[];
};

out VS_OUT {

  flat int index;
  vec2 opos;

} vs_out;

uniform float RATIO;
uniform mat4 vp;
uniform mat4 model;

void main() {

  vec4 tpos = vec4(0);
  int tind;
  if (in_Position.x > 0) tind = ind[gl_InstanceID].x;
  if (in_Position.y > 0) tind = ind[gl_InstanceID].y;
  if (in_Position.z > 0) tind = ind[gl_InstanceID].z;
  tpos = p[tind];
  //tpos.x /= RATIO;

  gl_Position = vp*model*tpos;
  vs_out.index = gl_InstanceID;
  vs_out.opos = o[tind];

}
