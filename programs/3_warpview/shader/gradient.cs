#version 460 core

layout(local_size_x = 1024) in;

layout (std430, binding = 1) buffer index {
  ivec4 ind[];
};

layout (std430, binding = 4) buffer energy {
  int en[];
};

layout (std430, binding = 5) buffer gradient {
  ivec2 gr[];
};

uniform int KTriangles;

void main(){

  const uint index = gl_GlobalInvocationID.x;
  if(index >= KTriangles)
    return;

  // Add the Non-Normalized Gradient to the Per-Vertex Gradients

  ivec2 xgrad = ivec2(
    en[ 1*KTriangles + index] - en[ 2*KTriangles + index],
    en[ 3*KTriangles + index] - en[ 4*KTriangles + index]
  );

  ivec2 ygrad = ivec2(
    en[ 5*KTriangles + index] - en[ 6*KTriangles + index],
    en[ 7*KTriangles + index] - en[ 8*KTriangles + index]
  );

  ivec2 zgrad = ivec2(
    en[ 9*KTriangles + index] - en[10*KTriangles + index],
    en[11*KTriangles + index] - en[12*KTriangles + index]
  );

  atomicAdd(gr[ind[index].x].x, xgrad.x /* + int(0.0f*ygrad.x) + int(0.0f*zgrad.x) */ );
  atomicAdd(gr[ind[index].x].y, xgrad.y /* + int(0.0f*ygrad.y) + int(0.0f*zgrad.y) */ );

  atomicAdd(gr[ind[index].y].x, ygrad.x /* + int(0.0f*zgrad.x) + int(0.0f*xgrad.x) */ );
  atomicAdd(gr[ind[index].y].y, ygrad.y /* + int(0.0f*zgrad.y) + int(0.0f*xgrad.y) */ );

  atomicAdd(gr[ind[index].z].x, zgrad.x /* + int(0.0f*xgrad.x) + int(0.0f*ygrad.x) */ );
  atomicAdd(gr[ind[index].z].y, zgrad.y /* + int(0.0f*xgrad.y) + int(0.0f*ygrad.y) */ );

}
