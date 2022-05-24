#version 460 core

in vec4 in_Position;
uniform float RATIO;
uniform mat4 vp;
uniform mat4 model;

void main(){
  vec4 p = in_Position;
  p.xy /= vec2(RATIO, 1);
  gl_Position = vp*model*p;
}
