#version 460 core

layout(local_size_x = 1024) in;

layout (std430, binding = 3) buffer energy {
  int en[];
};

layout (std430, binding = 4) buffer gradient {
  ivec2 gr[];
};

layout (std430, binding = 5) buffer indices {
  ivec4 ind[];
};

uniform int K;
uniform int mode;

void main(){

  const uint index = gl_GlobalInvocationID.x;
  if(index >= K)
    return;

  // Add the Non-Normalized Gradient to the Per-Vertex Gradients

  atomicAdd(gr[ind[index].x].x, en[ 1*K + index] - en[ 2*K + index]);
  atomicAdd(gr[ind[index].x].y, en[ 3*K + index] - en[ 4*K + index]);

  atomicAdd(gr[ind[index].y].x, en[ 5*K + index] - en[ 6*K + index]);
  atomicAdd(gr[ind[index].y].y, en[ 7*K + index] - en[ 8*K + index]);

  atomicAdd(gr[ind[index].z].x, en[ 9*K + index] - en[10*K + index]);
  atomicAdd(gr[ind[index].z].y, en[11*K + index] - en[12*K + index]);

}
