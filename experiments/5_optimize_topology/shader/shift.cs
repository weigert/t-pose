#version 460 core

layout(local_size_x = 1024) in;

layout (std430, binding = 0) buffer points {
  vec2 p[];
};

layout (std430, binding = 4) buffer gradient {
  ivec2 gr[];
};

uniform int NPoints;

void main(){

  const uint index = gl_GlobalInvocationID.x;

  if(index < 4)
    return;

  if(index >= NPoints)
    return;

  vec2 tgr = gr[index];

  if(p[index].x <= -6.0/9.0){
    p[index].x = -6.0/9.0;
    tgr.x = 0;
  }

  if(p[index].x >= 6.0/9.0){
    p[index].x = 6.0/9.0;
    tgr.x = 0;
  }

  if(p[index].y <= -1){
    p[index].y = -1;
    tgr.y = 0;
  }

  if(p[index].y >= 1){
    p[index].y = 1;
    tgr.y = 0;
  }

  tgr =  0.00005 * vec2(tgr) / 256 / 256;
  if(abs(tgr.x) < 1E-4) tgr.x = 0;
  if(abs(tgr.y) < 1E-4) tgr.y = 0;

  p[index] -= tgr;

}
