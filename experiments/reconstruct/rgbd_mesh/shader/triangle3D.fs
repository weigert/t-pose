#version 460 core

layout (std430, binding = 5) buffer othercol {
  vec4 ca[];
};

uniform sampler2D imageTexture;
uniform float RATIO;

in VS_OUT {
  flat int index;
  vec2 opos;
  flat vec3 normal;
} vs_out;

out vec4 fragColor;

void main(){

  vec2 p = vs_out.opos;
  p.x /= RATIO;
  p = (p+1.0f)*0.5;///vec2(960, 540);
  p.y = 1.0f-p.y;
//  p.y = 540-p.y;
//  p.x = -p.x;
//  p = (p/vec2(960, 540)*2.0f-1.0f)*vec2(960.0/540.0, 1.0f);


  // Display

  //fragColor = vec4(vec3(ca[vs_out.index].rgb), 1.0);
  fragColor = texture(imageTexture, p);
  //fragColor = vec4(vs_out.normal, 1.0f);
  //fragColor = vec4(1,0,0,0.1);

}
