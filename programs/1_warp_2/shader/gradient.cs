#version 460 core

layout(local_size_x = 1024) in;

layout (std430, binding = 1) buffer index {
  ivec4 ind[];
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

uniform int KTriangles;

void main(){

  const uint index = gl_GlobalInvocationID.x;
  if(index >= KTriangles)
    return;

  // Add the Non-Normalized Gradient to the Per-Vertex Gradients

  ivec2 xgrad = ivec2(
    ten[ 1*KTriangles + index] - ten[ 2*KTriangles + index],
    ten[ 3*KTriangles + index] - ten[ 4*KTriangles + index]
  );

  ivec2 ygrad = ivec2(
    ten[ 5*KTriangles + index] - ten[ 6*KTriangles + index],
    ten[ 7*KTriangles + index] - ten[ 8*KTriangles + index]
  );

  ivec2 zgrad = ivec2(
    ten[ 9*KTriangles + index] - ten[10*KTriangles + index],
    ten[11*KTriangles + index] - ten[12*KTriangles + index]
  );

  atomicAdd(gr[ind[index].x].x, xgrad.x /* + int(0.0f*ygrad.x) + int(0.0f*zgrad.x) */ );
  atomicAdd(gr[ind[index].x].y, xgrad.y /* + int(0.0f*ygrad.y) + int(0.0f*zgrad.y) */ );

  atomicAdd(gr[ind[index].y].x, ygrad.x /* + int(0.0f*zgrad.x) + int(0.0f*xgrad.x) */ );
  atomicAdd(gr[ind[index].y].y, ygrad.y /* + int(0.0f*zgrad.y) + int(0.0f*xgrad.y) */ );

  atomicAdd(gr[ind[index].z].x, zgrad.x /* + int(0.0f*xgrad.x) + int(0.0f*ygrad.x) */ );
  atomicAdd(gr[ind[index].z].y, zgrad.y /* + int(0.0f*xgrad.y) + int(0.0f*ygrad.y) */ );

}
