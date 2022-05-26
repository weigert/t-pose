#version 430 core

in float d;

out vec4 fragColor;

void main(){
  //fragColor = vec4(1,1,1,0.5);
  if( d < 0.0000002) fragColor = vec4(0,1,0,1);
  else fragColor = vec4(1,0,0,1);
 //fragColor = vec4(mix(vec3(0,1,0), vec3(1,0,0), d/0.000001), 1.0);
}
