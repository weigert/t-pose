#version 460 core

layout(local_size_x = 1024) in;

layout (std430, binding = 0) buffer points {
  vec2 p[];
};

layout (std430, binding = 6) buffer gradient {
  ivec2 gr[];
};

uniform int NPoints;
uniform float RATIO;

void main(){

  const uint index = gl_GlobalInvocationID.x;

  if(index < 4 || index >= NPoints)
    return;

  vec2 tgr = gr[index];

  if(p[index].x <= -RATIO){
    p[index].x = -RATIO;
    tgr.x = 0;
  }

  else if(p[index].x >= RATIO){
    p[index].x = RATIO;
    tgr.x = 0;
  }

  if(p[index].y <= -1){
    p[index].y = -1;
    tgr.y = 0;
  }

  else if(p[index].y >= 1){
    p[index].y = 1;
    tgr.y = 0;
  }

  p[index] -=  0.00005 * tgr / 256 / 256;

}
