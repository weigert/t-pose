#version 460 core

in vec2 ex_Tex;

out vec4 fragColor;

uniform sampler2D imageTexture;

void main(){
  fragColor = texture(imageTexture, ex_Tex);
}
