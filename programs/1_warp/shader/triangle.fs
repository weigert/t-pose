#version 460 core

layout (std430, binding = 2) buffer colacc {
  ivec4 ca[];
};

layout (std430, binding = 3) buffer colnum {
  int cn[];
};

layout (std430, binding = 4) buffer energy {
  int en[];
};

uniform sampler2D imageTexture;
uniform int mode;

in VS_OUT {
  vec2 position;
  flat int index;
} vs_out;

out vec4 fragColor;

void main(){

  // Accumulate Cost Function

  if( mode == 0) {

    atomicAdd(cn[vs_out.index], 1);

  }

  if( mode == 1 ){

    vec3 d = 255*texture(imageTexture, vs_out.position).rgb - vec3(ca[vs_out.index].rgb);
    atomicAdd(en[vs_out.index], int(0.5*dot(d, d)));

  }

  // Display

  if( mode == 2 ){

   fragColor = vec4(vec3(ca[vs_out.index].rgb)/255, 1);

  }

}
