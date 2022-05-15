#version 460 core

layout (std430, binding = 1) buffer colacc {
  ivec4 ca[];
};

layout (std430, binding = 2) buffer colnum {
  int cn[];
};

uniform sampler2D imageTexture;

in vec3 ex_Color;
in vec2 ex_Position;
flat in int ex_Index;

uniform bool renderaverage = false;

out vec4 fragColor;

void main(){

  // Accumulate the Guy

  fragColor = texture(imageTexture, ex_Position);

  if(renderaverage)
    fragColor = vec4(vec3(ca[ex_Index].rgb)/256, 1.0);

  else {

    atomicAdd(cn[ex_Index], 1);
    atomicAdd(ca[ex_Index].r, int(256*fragColor.r));
    atomicAdd(ca[ex_Index].g, int(256*fragColor.g));
    atomicAdd(ca[ex_Index].b, int(256*fragColor.b));

  }

}
