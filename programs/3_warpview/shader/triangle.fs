#version 460 core

layout (std430, binding = 2) buffer colacc {
  ivec4 ca[];
};

uniform sampler2D imageTexture;
uniform int mode;

in VS_OUT {
  vec2 position;
  vec2 opos;
  flat int index;
} vs_out;

out vec4 fragColor;

void main(){

  // Display

  if( mode == 2 ){

  //fragColor = vec4(vec3(ca[vs_out.index].rgb)/255, 1);
    fragColor = texture(imageTexture, vs_out.opos);
    //fragColor = vec4(0,0,0,1);

  }

}
