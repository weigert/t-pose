#version 460 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 128) out;

uniform int KTriangles;
const float dp = 0.01;

in VS_OUT {
  vec2 position;
  flat int index;
} gs_in [];

out GS_OUT {
  vec2 position;
  flat int index;
} gs_out;

uniform int mode;

void main() {

  vec2 tpos[3];
  int index;

  // Original Triangle

  tpos[0] = gs_in[0].position;
  tpos[1] = gs_in[1].position;
  tpos[2] = gs_in[2].position;
  index = gs_in[0].index;

  gl_Position = vec4(tpos[0], -1, 1.0f);
  gs_out.position = vec2(0.5*(1.0+tpos[0].x), 0.5*(1.0-tpos[0].y));
  gs_out.index = index;
  EmitVertex();

  gl_Position = vec4(tpos[1], -1, 1.0f);
  gs_out.position = vec2(0.5*(1.0+tpos[1].x), 0.5*(1.0-tpos[1].y));
  gs_out.index = index;
  EmitVertex();

  gl_Position = vec4(tpos[2], -1, 1.0f);
  gs_out.position = vec2(0.5*(1.0+tpos[2].x), 0.5*(1.0-tpos[2].y));
  gs_out.index = index;
  EmitVertex();



  if(mode != 3)
  for(int i = 0; i < 12; i++){

    // Shift Triangle Vertices

    tpos[0] = gs_in[0].position;
    tpos[1] = gs_in[1].position;
    tpos[2] = gs_in[2].position;

    if(i == 0) tpos[0]  += vec2(dp, 0);
    if(i == 1) tpos[0]  -= vec2(dp, 0);
    if(i == 2) tpos[0]  += vec2(0, dp);
    if(i == 3) tpos[0]  -= vec2(0, dp);
    if(i == 4) tpos[1]  += vec2(dp, 0);
    if(i == 5) tpos[1]  -= vec2(dp, 0);
    if(i == 6) tpos[1]  += vec2(0, dp);
    if(i == 7) tpos[1]  -= vec2(0, dp);
    if(i == 8) tpos[2]  += vec2(dp, 0);
    if(i == 9) tpos[2]  -= vec2(dp, 0);
    if(i == 10) tpos[2] += vec2(0, dp);
    if(i == 11) tpos[2] -= vec2(0, dp);

    index = (1+i)*KTriangles + gs_in[0].index;

    // Degenerate Vertex

    EmitVertex();

    gl_Position = vec4(tpos[0], -1, 1.0f);
    gs_out.position = vec2(0.5*(1.0+tpos[0].x), 0.5*(1.0-tpos[0].y));
    gs_out.index = index;
    EmitVertex();
    EmitVertex();

    gl_Position = vec4(tpos[1], -1, 1.0f);
    gs_out.position = vec2(0.5*(1.0+tpos[1].x), 0.5*(1.0-tpos[1].y));
    gs_out.index = index;
    EmitVertex();

    gl_Position = vec4(tpos[2], -1, 1.0f);
    gs_out.position = vec2(0.5*(1.0+tpos[2].x), 0.5*(1.0-tpos[2].y));
    gs_out.index = index;
    EmitVertex();

  }

  EndPrimitive();

}
