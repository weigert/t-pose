#version 460 core

in vec2 in_Position;

void main(){
  vec2 p = 2*(vec2(in_Position.x/1200,1.0f-in_Position.y/675)-0.5);
  gl_Position = vec4(p, -1.0f, 1.0f);
}
