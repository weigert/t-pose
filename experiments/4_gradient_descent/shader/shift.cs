#version 460 core

layout(local_size_x = 1024) in;

layout (std430, binding = 0) buffer points {
  vec2 p[];
};

layout (std430, binding = 4) buffer gradient {
  ivec2 gr[];
};

uniform int NPoints;
uniform int mode;

void main(){

  const uint index = gl_GlobalInvocationID.x;

  if(index < 4)
    return;

  if(index >= NPoints)
    return;

  p[index] -= 0.0001 * vec2(gr[index]) / 256 / 256;

  if(p[index].x < -12.0/8.0) p[index].x = -12.0/8.0;
  if(p[index].x > 12.0/8.0) p[index].x = 12.0/8.0;
  if(p[index].y < -1) p[index].y = -1;
  if(p[index].y > 1) p[index].y = 1;

}
