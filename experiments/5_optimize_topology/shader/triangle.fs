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
uniform int mode;

in GS_OUT {
  vec2 position;
  flat int index;
} gs_out;

out vec4 fragColor;

void main(){

  if( mode == 0 ){

    // Accumulate Color

    fragColor = texture(imageTexture, gs_out.position);
    atomicAdd(cn[gs_out.index], 1);
    atomicAdd(ca[gs_out.index].r, int(255*fragColor.r));
    atomicAdd(ca[gs_out.index].g, int(255*fragColor.g));
    atomicAdd(ca[gs_out.index].b, int(255*fragColor.b));

  }

  if( mode == 1 || mode == 3 ){

    // Accumulate Cost Function

    vec3 d = 255*texture(imageTexture, gs_out.position).rgb - vec3(ca[gs_out.index].rgb);
    atomicAdd(en[gs_out.index], int(0.5*dot(d, d)));

  }

  if( mode == 2 ){

    // Display

  //  fragColor = mix(vec4(0,0,1,1), vec4(1,0,0,1), sqrt(float(en[gs_out.index]/cn[gs_out.index]))/255.0f);
   fragColor = vec4(vec3(ca[gs_out.index].rgb)/255, 1);
  //  fragColor = vec4(0,0,0,1);

  }

}
