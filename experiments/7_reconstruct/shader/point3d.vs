#version 460 core

in vec4 in_Position;
uniform mat4 vp;
uniform mat4 model;

void main(){

  vec4 p = in_Position;
  p.xyz /= p.w;
  //p.y = -p.y;
//  p.x = -p.x;
  //p.y = -p.y;
  //p.x = -p.x;
  //gl_Position = vec4(p, -1.0f, 1.0f);

  //p = 2*(vec2(in_Position.x, 1.0f-in_Position.y/(675.0/1200.0))-0.5);


  gl_Position = vp*model*p;
}
