#version 460 core

layout(location = 0) in vec4 in_Position;
layout(location = 1) in vec4 in_Color;
layout(location = 2) in vec4 in_Normal;

out vec4 ex_Color;
uniform mat4 vp;

void main() {

  ex_Color = vec4(abs(normalize(in_Normal.xyz)), 1.0f);
//  ex_Color = in_Color;
  gl_Position = vp*in_Position;

}
