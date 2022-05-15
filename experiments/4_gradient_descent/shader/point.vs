#version 460 core

in vec2 in_Position;

void main(){
  gl_Position = vec4(in_Position/vec2(1.2/0.8, 1), -1.0f, 1.0f);
}
