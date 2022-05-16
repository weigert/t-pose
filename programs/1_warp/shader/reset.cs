#version 460 core

layout(local_size_x = 1024) in;

layout (std430, binding = 2) buffer colacc {
  ivec4 ca[];
};

layout (std430, binding = 3) buffer colnum {
  int cn[];
};

layout (std430, binding = 4) buffer energy {
  int en[];
};

layout (std430, binding = 5) buffer gradient {
  ivec2 gr[];
};


uniform int NTriangles;
uniform int NPoints;

void main(){

  const uint index = gl_GlobalInvocationID.x;

  //if(index < NTriangles){

    en[index] = 0;

  //}

  //if(index < NPoints){

    gr[index] = ivec2(0);

  //}

};
