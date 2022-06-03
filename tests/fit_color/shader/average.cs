#version 460 core

layout(local_size_x = 1024) in;

layout (std430, binding = 1) buffer colacc {
  ivec4 ca[];
};

layout (std430, binding = 2) buffer colnum {
  int cn[];
};

uniform int N;

void main(){

  const uint index = gl_GlobalInvocationID.x;

  if(index >= N)
    return;

  if(cn[index] > 0)
    ca[index] = ca[index]/cn[index];

};
