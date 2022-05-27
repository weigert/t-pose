#version 460 core

in vec3 in_Position;

layout (std430, binding = 0) buffer points {
  vec2 p[];
};

layout (std430, binding = 1) buffer index {
  ivec4 ind[];
};

layout (std430, binding = 2) buffer colacc {
  ivec4 ca[];
};

layout (std430, binding = 3) buffer colnum {
  int cn[];
};

layout (std430, binding = 4) buffer tenergy {
  int ten[];
};

layout (std430, binding = 6) buffer gradient {
  ivec2 gr[];
};

layout (std430, binding = 7) buffer nring {
  int nr[];
};

out VS_OUT {

  vec2 position;
  flat int index;

} vs_out;

uniform float RATIO;
uniform int KTriangles;
uniform int mode;

void main() {

  const int TDIV = gl_InstanceID/13;
  const int TMOD = gl_InstanceID%13;
  vs_out.index = gl_InstanceID;

  int pind;               // Vertex Index
  if (in_Position.x > 0)
    pind = ind[TDIV].x;
  if (in_Position.y > 0)
    pind = ind[TDIV].y;
  if (in_Position.z > 0)
    pind = ind[TDIV].z;

  vec2 tpos = p[pind];    // Vertex Image-Space (-RATIO, RATIO) x, (-1, 1) y
  float dp = 0.05f; // Image-Space Pixel Shift

  // Cooling Regiment:

  // 1.: Divide by 2 as KTriangles -> 1000
  //dp /= (1.0f + 1.0f*float(KTriangles)/1000.0f);

  // 2.: Divide by 3 as KTriangles -> 1000
  //dp /= (1.0f + 2.0f*float(KTriangles)/1000.0f);

  // 3.: Divide by 10 as KTriangles -> 1000
  dp /= (1.0f + 9.0f*float(KTriangles)/1000.0f);

  // 4.: Divide by 20 as KTriangles -> 1000
  //dp /= (1.0f + 19.0f*float(KTriangles)/1000.0f);

  // 5.: Divide by 50 as KTriangles -> 1000
  //dp /= (1.0f + 49.0f*float(KTriangles)/1000.0f);

  // 6.: Scale by Size of Triangle!!
  //dp = 0.5*0.5*abs(determinant(mat3(
  //  p[ind[TMOD].x].x, p[ind[TMOD].x].y, 1,
  //  p[ind[TMOD].y].x, p[ind[TMOD].y].y, 1,
  //  p[ind[TMOD].z].x, p[ind[TMOD].z].y, 1
  //)));

  // 7.: Scale by Eigenspace Projection

  /*
  vec2 va, vb;
  if(in_Position.x > 0){
    va = (p[ind[TMOD].y] - p[ind[TMOD].x]);
    vb = (p[ind[TMOD].z] - p[ind[TMOD].x]);
  }
  if(in_Position.y > 0){
    va = (p[ind[TMOD].z] - p[ind[TMOD].y]);
    vb = (p[ind[TMOD].x] - p[ind[TMOD].y]);
  }
  if(in_Position.z > 0){
    va = (p[ind[TMOD].x] - p[ind[TMOD].z]);
    vb = (p[ind[TMOD].y] - p[ind[TMOD].z]);
  }
  mat2 V = transpose(mat2(normalize(va), normalize(vb)));
  mat2 L = mat2(length(va), 0, 0, length(vb));

  vec2 D = V*L*inverse(V)*vec2(dp);

  */

  vec2 D = vec2(dp);

       if(TMOD == 1 && in_Position.x > 0)   D *= vec2( 1, 0);
  else if(TMOD == 2 && in_Position.x > 0)   D *= vec2(-1, 0);
  else if(TMOD == 3 && in_Position.x > 0)   D *= vec2( 0, 1);
  else if(TMOD == 4 && in_Position.x > 0)   D *= vec2( 0,-1);
  else if(TMOD == 5 && in_Position.y > 0)   D *= vec2( 1, 0);
  else if(TMOD == 6 && in_Position.y > 0)   D *= vec2(-1, 0);
  else if(TMOD == 7 && in_Position.y > 0)   D *= vec2( 0, 1);
  else if(TMOD == 8 && in_Position.y > 0)   D *= vec2( 0,-1);
  else if(TMOD == 9 && in_Position.z > 0)   D *= vec2( 1, 0);
  else if(TMOD == 10 && in_Position.z > 0)  D *= vec2(-1, 0);
  else if(TMOD == 11 && in_Position.z > 0)  D *= vec2( 0, 1);
  else if(TMOD == 12 && in_Position.z > 0)  D *= vec2( 0,-1);
  else D *= vec2(0);

  tpos += D;
  tpos.x /= RATIO;  // Position in Screen-Space (-1, 1) for x,y

  gl_Position = vec4(tpos, -1, 1.0f);
  vs_out.position = vec2(0.5*(1.0+tpos.x), 0.5*(1.0-tpos.y));

  if(mode == 0){
    cn[vs_out.index] = 0;
    ca[vs_out.index] = ivec4(0);
  }

  if(mode == 1){
    ten[vs_out.index] = 0;
    gr[ind[TDIV].x] = ivec2(0);
    gr[ind[TDIV].y] = ivec2(0);
    gr[ind[TDIV].z] = ivec2(0);
  }

  //Add One-Ring Energy

  /*

  if( TDIV == 0 && mode == 0 )
    atomicAdd(nr[TMOD], 1 );    //count the n-ring! (divided by 3)

  if( TDIV == 0 && mode == 1 ) {

    const float lambda = 0*256*256;

    if (in_Position.x > 0){

      vec2 wva = p[pind] - p[ind[TMOD].y]; //Distance from this Vertex to Prev
      vec2 wvb = p[pind] - p[ind[TMOD].z]; //Distance from this Vertex to Next
      //Add Direct Energy Gradient
      atomicAdd(gr[pind].x, int(lambda*0.5/3.0*(dot(wva+vec2(dp, 0), wva+vec2(dp, 0)) - dot(wva-vec2(dp, 0), wva-vec2(dp, 0))))/nr[TMOD]);
      atomicAdd(gr[pind].x, int(lambda*0.5/3.0*(dot(wvb+vec2(dp, 0), wvb+vec2(dp, 0)) - dot(wvb-vec2(dp, 0), wvb-vec2(dp, 0))))/nr[TMOD]);
      atomicAdd(gr[pind].y, int(lambda*0.5/3.0*(dot(wva+vec2( 0,dp), wva+vec2( 0,dp)) - dot(wva-vec2( 0,dp), wva-vec2( 0,dp))))/nr[TMOD]);
      atomicAdd(gr[pind].y, int(lambda*0.5/3.0*(dot(wvb+vec2( 0,dp), wvb+vec2( 0,dp)) - dot(wvb-vec2( 0,dp), wvb-vec2( 0,dp))))/nr[TMOD]);

    }

    if (in_Position.y > 0){

      vec2 wva = p[pind] - p[ind[TMOD].z]; //Distance from this Vertex to Prev
      vec2 wvb = p[pind] - p[ind[TMOD].x]; //Distance from this Vertex to Next
      //Add Direct Energy Gradient
      atomicAdd(gr[pind].x, int(lambda*0.5/3.0*(dot(wva+vec2(dp, 0), wva+vec2(dp, 0)) - dot(wva-vec2(dp, 0), wva-vec2(dp, 0))))/nr[TMOD]);
      atomicAdd(gr[pind].x, int(lambda*0.5/3.0*(dot(wvb+vec2(dp, 0), wvb+vec2(dp, 0)) - dot(wvb-vec2(dp, 0), wvb-vec2(dp, 0))))/nr[TMOD]);
      atomicAdd(gr[pind].y, int(lambda*0.5/3.0*(dot(wva+vec2( 0,dp), wva+vec2( 0,dp)) - dot(wva-vec2( 0,dp), wva-vec2( 0,dp))))/nr[TMOD]);
      atomicAdd(gr[pind].y, int(lambda*0.5/3.0*(dot(wvb+vec2( 0,dp), wvb+vec2( 0,dp)) - dot(wvb-vec2( 0,dp), wvb-vec2( 0,dp))))/nr[TMOD]);

    }

    if (in_Position.z > 0){

      vec2 wva = p[pind] - p[ind[TMOD].x]; //Distance from this Vertex to Prev
      vec2 wvb = p[pind] - p[ind[TMOD].y]; //Distance from this Vertex to Next
      //Add Direct Energy Gradient
      atomicAdd(gr[pind].x, int(lambda*0.5/3.0*(dot(wva+vec2(dp, 0), wva+vec2(dp, 0)) - dot(wva-vec2(dp, 0), wva-vec2(dp, 0))))/nr[TMOD]);
      atomicAdd(gr[pind].x, int(lambda*0.5/3.0*(dot(wvb+vec2(dp, 0), wvb+vec2(dp, 0)) - dot(wvb-vec2(dp, 0), wvb-vec2(dp, 0))))/nr[TMOD]);
      atomicAdd(gr[pind].y, int(lambda*0.5/3.0*(dot(wva+vec2( 0,dp), wva+vec2( 0,dp)) - dot(wva-vec2( 0,dp), wva-vec2( 0,dp))))/nr[TMOD]);
      atomicAdd(gr[pind].y, int(lambda*0.5/3.0*(dot(wvb+vec2( 0,dp), wvb+vec2( 0,dp)) - dot(wvb-vec2( 0,dp), wvb-vec2( 0,dp))))/nr[TMOD]);

    }

  }

  */

}
