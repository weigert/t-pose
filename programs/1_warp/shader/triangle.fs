#version 460 core

layout (std430, binding = 1) buffer index {
  ivec4 ind[];
};

layout (std430, binding = 2) buffer colacc {
  ivec4 ca[];
};

layout (std430, binding = 3) buffer colnum {
  int cn[];
};

layout (std430, binding = 4) buffer tenergy {
  int ten[];
};

layout (std430, binding = 5) buffer penergy {
  int pen[];
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
    atomicAdd(ten[vs_out.index], int(0.5*dot(d, d)));

  }

  // Display

  if( mode == 2 ){

    /*
    float TEN = sqrt(float(ten[vs_out.index])/cn[vs_out.index])/255.0f;
    float PEN = 0.0f;
    PEN += pen[ind[vs_out.index].x];
    PEN += pen[ind[vs_out.index].y];
    PEN += pen[ind[vs_out.index].z];

    if(PEN == 0) fragColor = vec4(0,1,0,1);
    else if(abs(1500*TEN) > abs(PEN)) fragColor = vec4(1,0,0,1);
    else fragColor = vec4(0,0,1,1);

    */

  //  fragColor = mix(vec4(0,0.5,1,1), vec4(1,0.5,0,1), );
    fragColor = vec4(vec3(ca[vs_out.index].rgb)/255, 1);

  }

}
