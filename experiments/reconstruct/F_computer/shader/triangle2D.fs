#version 430 core

flat in int ex_Index;
out vec4 fragColor;

uniform bool docolor;

layout (std430, binding = 1) buffer colacc {
  ivec4 col[];
};

layout (std430, binding = 4) buffer selected {
  int s[];
};

void main(){

  if(!docolor) fragColor = vec4(1,1,1, 0.5);

  else if(s[ex_Index] == 1)
    fragColor = vec4(col[ex_Index].xyz, 255)/255.0f;

  else fragColor = vec4(0,0,0,1);

}
