#version 460 core

layout(local_size_x = 1024) in;

layout (std430, binding = 2) buffer colacc {
  ivec4 ca[];
};

layout (std430, binding = 3) buffer colnum {
  int cn[];
};

layout (std430, binding = 4) buffer tenergy {
  int ten[];
};

layout (std430, binding = 5) buffer penergy {
  int pen[];
};

layout (std430, binding = 6) buffer gradient {
  ivec2 gr[];
};

layout (std430, binding = 7) buffer nring {
  int nr[];
};


uniform int NTriangles;
uniform int NPoints;

void main(){

  const uint index = gl_GlobalInvocationID.x;

  //if(index < NTriangles){
    cn[index] = 0;
    ten[index] = 0;
    pen[index] = 0;

  //}

  //if(index < NPoints){

    gr[index] = ivec2(0);
    nr[index] = 0;

  //}

};
