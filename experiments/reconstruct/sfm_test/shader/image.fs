#version 330
in vec2 ex_Tex;
out vec4 fragColor;

uniform sampler2D imageTextureA;
uniform sampler2D imageTextureB;

uniform bool flip;

void main(){
  if(flip) fragColor = texture(imageTextureA, ex_Tex);
  else fragColor = texture(imageTextureB, ex_Tex);
}
