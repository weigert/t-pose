#version 460 core

layout (std430, binding = 1) buffer colacc {
  ivec4 ca[];
};

layout (std430, binding = 2) buffer colnum {
  int cn[];
};

layout (std430, binding = 3) buffer energy {
  int en[];
};

uniform sampler2D imageTexture;

in vec3 ex_Color;
in vec2 ex_Position;
flat in int ex_Index;

uniform int mode;

out vec4 fragColor;

void main(){

  if( mode == 0 ){

    // Accumulate Color

    fragColor = texture(imageTexture, ex_Position);
    atomicAdd(cn[ex_Index], 1);
    atomicAdd(ca[ex_Index].r, int(256*fragColor.r));
    atomicAdd(ca[ex_Index].g, int(256*fragColor.g));
    atomicAdd(ca[ex_Index].b, int(256*fragColor.b));

  }

  if( mode == 1 ){

    // Accumulate Cost Function

    atomicAdd(en[ex_Index], int(length(vec3(ca[ex_Index].rgb) - 256*texture(imageTexture, ex_Position).rgb)));

  }

  if( mode == 2 ){

    // Display

    fragColor = mix(vec4(1,0,0,1), vec4(0,0,1,1), float(en[ex_Index])/256);

  }

}
