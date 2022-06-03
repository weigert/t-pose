#version 460 core

in vec3 in_Position;
in ivec4 in_Index;

flat out int ex_Index;

layout (std430, binding = 0) buffer points {
  vec2 p[];
};

uniform float RATIO;

out vec3 ex_Color;
out vec2 ex_Position;

vec3 color(int i){
  float r = ((i >>  0) & 0xff)/255.0f;
  float g = ((i >>  8) & 0xff)/255.0f;
  float b = ((i >> 16) & 0xff)/255.0f;
  return vec3(r,g,b);
}

void main() {

  vec2 tpos = vec2(0);
  if (in_Position.x > 0) tpos = p[in_Index.x];
  if (in_Position.y > 0) tpos = p[in_Index.y];
  if (in_Position.z > 0) tpos = p[in_Index.z];
  tpos.x /= RATIO;

  ex_Position = vec2(0.5*(1.0+tpos.x), 0.5*(1.0-tpos.y));
  gl_Position = vec4(tpos, -1, 1.0f);

  ex_Color = color(gl_InstanceID);
  ex_Index = gl_InstanceID;

}
