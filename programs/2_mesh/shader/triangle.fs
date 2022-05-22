#version 460 core

layout (std430, binding = 2) buffer colacc {
  ivec4 ca[];
};

uniform sampler2D imageTexture;

in VS_OUT {
  flat int index;
} vs_out;

out vec4 fragColor;

void main(){

  // Display

  fragColor = vec4(vec3(ca[vs_out.index].rgb)/255, 1.0);
  //  fragColor = texture(imageTexture, vs_out.opos);
    //fragColor = vec4(0,0,0,1);

}
