#version 460 core

layout(local_size_x = 1024) in;

layout (std430, binding = 1) buffer index {
  ivec4 ind[];
};

layout (std430, binding = 4) buffer tenergy {
  int ten[];
};

layout (std430, binding = 6) buffer gradient {
  ivec2 gr[];
};

uniform int KTriangles;

void main(){

  const uint index = gl_GlobalInvocationID.x;
  if(index >= KTriangles)
    return;

  // Add the Non-Normalized Gradient to the Per-Vertex Gradients

  atomicAdd(gr[ind[index].x].x, ten[ 1*KTriangles + index] - ten[ 2*KTriangles + index]);
  atomicAdd(gr[ind[index].x].y, ten[ 3*KTriangles + index] - ten[ 4*KTriangles + index]);

  atomicAdd(gr[ind[index].y].x, ten[ 5*KTriangles + index] - ten[ 6*KTriangles + index]);
  atomicAdd(gr[ind[index].y].y, ten[ 7*KTriangles + index] - ten[ 8*KTriangles + index]);

  atomicAdd(gr[ind[index].z].x, ten[ 9*KTriangles + index] - ten[10*KTriangles + index]);
  atomicAdd(gr[ind[index].z].y, ten[11*KTriangles + index] - ten[12*KTriangles + index]);

}
