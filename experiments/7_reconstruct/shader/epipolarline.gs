#version 460 core

layout (points) in;
layout (line_strip, max_vertices = 2) out;

in VS_OUT {
  vec3 P;
} gs_in [];

uniform int mode;

void main() {

  vec3 P = gs_in[0].P;
  float x = 0;
  vec2 p = vec2(x, P.z/P.y + x*P.x/P.y);

  p = 2*vec2(p.x/1200,1+p.y/675)-1;


  gl_Position = vec4(p.x, p.y, -1, 1.0f);
  EmitVertex();

  P = gs_in[0].P;
  x = 2400;
  p = vec2(x, P.z/P.y + x*P.x/P.y);
  p = 2*vec2(p.x/1200,1+p.y/675)-1;


  gl_Position = vec4(p.x, p.y, -1, 1.0f);
  EmitVertex();

  EndPrimitive();

}
