#version 460 core

layout(local_size_x = 1024) in;

layout (std430, binding = 1) buffer index {
  ivec4 ind[];
};

layout (std430, binding = 4) buffer tenergy {
  int en[];
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

  /*
  atomicAdd(gr[ind[index].x].x, en[ 1*KTriangles + index] - en[ 2*KTriangles + index]);
  atomicAdd(gr[ind[index].x].y, en[ 3*KTriangles + index] - en[ 4*KTriangles + index]);

  atomicAdd(gr[ind[index].y].x, en[ 5*KTriangles + index] - en[ 6*KTriangles + index]);
  atomicAdd(gr[ind[index].y].y, en[ 7*KTriangles + index] - en[ 8*KTriangles + index]);

  atomicAdd(gr[ind[index].z].x, en[ 9*KTriangles + index] - en[10*KTriangles + index]);
  atomicAdd(gr[ind[index].z].y, en[11*KTriangles + index] - en[12*KTriangles + index]);
  */

  atomicAdd(gr[ind[index].x].x, en[ 13*index + 1 ] - en[ 13*index + 2 ]);
  atomicAdd(gr[ind[index].x].y, en[ 13*index + 3 ] - en[ 13*index + 4 ]);

  atomicAdd(gr[ind[index].y].x, en[ 13*index + 5 ] - en[ 13*index + 6 ]);
  atomicAdd(gr[ind[index].y].y, en[ 13*index + 7 ] - en[ 13*index + 8 ]);

  atomicAdd(gr[ind[index].z].x, en[ 13*index + 9 ] - en[ 13*index + 10]);
  atomicAdd(gr[ind[index].z].y, en[ 13*index + 11] - en[ 13*index + 12]);

}
