#version 460 core

in vec2 in_Position;
uniform float RATIO;

void main(){
  gl_Position = vec4(in_Position/vec2(RATIO, 1), -1.0f, 1.0f);
}
