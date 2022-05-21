#version 460 core

in vec4 in_Position;
uniform mat4 vp;

void main(){
  vec4 p = vp*in_Position;
  //p.y = 1.0f-p.y;
  //p.x = -p.x;
  //gl_Position = vec4(p, -1.0f, 1.0f);

  gl_Position = p;
}
