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

uniform float RATIO;
uniform float s;

void main() {

  vec2 tpos = vec2(0);
  if (in_Position.x > 0) tpos = mix(p[ind[gl_InstanceID].x], op[ind[gl_InstanceID].x], s);
  if (in_Position.y > 0) tpos = mix(p[ind[gl_InstanceID].y], op[ind[gl_InstanceID].y], s);
  if (in_Position.z > 0) tpos = mix(p[ind[gl_InstanceID].z], op[ind[gl_InstanceID].z], s);
  tpos.x /= RATIO;
  gl_Position = vec4(tpos, -1, 1.0f);

}
