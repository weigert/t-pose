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
uniform int mode;

void main(){

  const uint index = gl_GlobalInvocationID.x;

  if(index >= NTriangles)
    return;

  if(mode == 0){

    if(cn[index] > 0)
      ca[index] = ca[index]/cn[index];

  }

  if(mode == 1){

  //  if(cn[index] > 0)
  //    en[index] = en[index]/cn[index];
  //  else en[index] = 0;

  }

};
