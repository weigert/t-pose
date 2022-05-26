#version 460 core

layout (std430, binding = 2) buffer colacc {
  ivec4 ca[];
};

layout (std430, binding = 3) buffer colnum {
  int cn[];
};

layout (std430, binding = 4) buffer tenergy {
  int ten[];
};

uniform sampler2D imageTexture;
uniform int mode;

in VS_OUT {
  vec2 position;
  flat int index;
} vs_out;

out vec4 fragColor;

void main(){

  if( mode == 0 ){  // Accumulate Color

    fragColor = texture(imageTexture, vs_out.position);
    atomicAdd(cn[vs_out.index], 1);
    atomicAdd(ca[vs_out.index].r, int(255*fragColor.r));
    atomicAdd(ca[vs_out.index].g, int(255*fragColor.g));
    atomicAdd(ca[vs_out.index].b, int(255*fragColor.b));

  }

  if( mode == 1 ){  // Accumulate Energy
    
    vec3 d = vec3(0);
    if(cn[vs_out.index] > 0) d = 255*texture(imageTexture, vs_out.position).rgb - vec3(ca[vs_out.index].rgb/cn[vs_out.index]);
    atomicAdd(ten[vs_out.index], int(0.5*dot(d, d)));

  }

  if( mode == 2 ){  // Display

  //  fragColor = mix(vec4(0,0,1,1), vec4(1,0,0,1), sqrt(float(en[vs_out.index]/cn[vs_out.index]))/255.0f);
   fragColor = vec4(vec3(ca[vs_out.index].rgb)/cn[vs_out.index]/255, 1);

  }

}
