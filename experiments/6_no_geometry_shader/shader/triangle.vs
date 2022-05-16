#version 460 core

in vec3 in_Position;
in ivec4 in_Index;

layout (std430, binding = 0) buffer points {
  vec2 p[];
};

layout (std430, binding = 1) buffer index {
  ivec4 ind[];
};

out VS_OUT {

  vec2 position;
  flat int index;

} vs_out;

uniform float RATIO;
uniform int KTriangles;

void main() {

  vec2 tpos = vec2(0);
  float dp = 0.0f;
  int TDIV = gl_InstanceID/KTriangles;
  int TMOD = gl_InstanceID%KTriangles;
  vs_out.index = gl_InstanceID;

  if (in_Position.x > 0){
    tpos = p[ind[TMOD].x];
    dp = 0.05f;
  //  dp = max(0.01, 0.05*length(p[ind[TMOD].y]-p[ind[TMOD].z]));
  }
  if (in_Position.y > 0){
    tpos = p[ind[TMOD].y];
    dp = 0.05f;
  //  dp = max(0.01, 0.05*length(p[ind[TMOD].z]-p[ind[TMOD].x]));
  }
  if (in_Position.z > 0){
    tpos = p[ind[TMOD].z];
    dp = 0.05f;
  //  dp = max(0.01, 0.05*length(p[ind[TMOD].x]-p[ind[TMOD].y]));
  }
  tpos.x /= RATIO;


  if(TDIV == 1 && in_Position.x > 0) tpos  += vec2(dp, 0);
  else if(TDIV == 2 && in_Position.x > 0) tpos  -= vec2(dp, 0);
  else if(TDIV == 3 && in_Position.x > 0) tpos  += vec2( 0,dp);
  else if(TDIV == 4 && in_Position.x > 0) tpos  -= vec2( 0,dp);
  else if(TDIV == 5 && in_Position.y > 0) tpos  += vec2(dp, 0);
  else if(TDIV == 6 && in_Position.y > 0) tpos  -= vec2(dp, 0);
  else if(TDIV == 7 && in_Position.y > 0) tpos  += vec2( 0,dp);
  else if(TDIV == 8 && in_Position.y > 0) tpos  -= vec2( 0,dp);
  else if(TDIV == 9 && in_Position.z > 0) tpos  += vec2(dp, 0);
  else if(TDIV == 10 && in_Position.z > 0) tpos -= vec2(dp, 0);
  else if(TDIV == 11 && in_Position.z > 0) tpos += vec2( 0,dp);
  else if(TDIV == 12 && in_Position.z > 0) tpos -= vec2( 0,dp);

  gl_Position = vec4(tpos, -1, 1.0f);
  vs_out.position = vec2(0.5*(1.0+tpos.x), 0.5*(1.0-tpos.y));

}
