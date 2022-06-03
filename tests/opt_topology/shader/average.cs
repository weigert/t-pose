#version 460 core

layout(local_size_x = 1024) in;

layout (std430, binding = 1) buffer colacc {
  ivec4 ca[];
};

layout (std430, binding = 2) buffer colnum {
  int cn[];
};

layout (std430, binding = 3) buffer energy {
  int en[];
};

uniform int NTriangles;

void main(){

  const uint index = gl_GlobalInvocationID.x;

  if(index >= NTriangles)
    return;

  if(cn[index] > 0)
    ca[index] = ca[index]/cn[index];

};
