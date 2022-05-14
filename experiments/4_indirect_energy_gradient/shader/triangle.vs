#version 460 core

in vec3 in_Position;
in ivec4 in_Index;

layout (std430, binding = 0) buffer points {
  vec2 p[];
};

out VS_OUT {

  vec2 position;
  flat int index;

} vs_out;

void main() {

  vec2 tpos = vec2(0);
  if (in_Position.x > 0) tpos = p[in_Index.x];
  if (in_Position.y > 0) tpos = p[in_Index.y];
  if (in_Position.z > 0) tpos = p[in_Index.z];
  tpos.x /= 1200.0f/800.0f;

  vs_out.position = tpos;
  vs_out.index = gl_InstanceID;

}
