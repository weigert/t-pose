#version 460 core

in vec3 in_Position;
in ivec4 in_Index;

layout (std430, binding = 0) buffer points {
  vec2 p[];
};

layout (std430, binding = 1) buffer index {
  ivec4 ind[];
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

  //Add One-Ring Energy

  if( TDIV == 0 && mode == 0 )
    atomicAdd(nr[TMOD], 1 );    //count the n-ring! (divided by 3)

  if( TDIV == 0 && mode == 1 ) {

    /*
        This shader is executed once per vertex of all triangles (including shifted ones).
        The instance ID is per triangle.

        I therefore take a look at all vertices of the original triangle (i.e. TDIV = 0),
        and compute the energy gradient of the one-ring energy with the known shifts.
    */

    const float lambda = 0.01*256*256;

    if (in_Position.x > 0){

      vec2 wva = p[ind[TMOD].x] - p[ind[TMOD].y]; //Distance from this Vertex to Prev
      vec2 wvb = p[ind[TMOD].x] - p[ind[TMOD].z]; //Distance from this Vertex to Next
      //Add Direct Energy Gradient
      atomicAdd(gr[ind[TMOD].x].x, int(lambda*0.5/3.0*(dot(wva+vec2(dp, 0), wva+vec2(dp, 0)) - dot(wva-vec2(dp, 0), wva-vec2(dp, 0))))/nr[TMOD]);
      atomicAdd(gr[ind[TMOD].x].x, int(lambda*0.5/3.0*(dot(wvb+vec2(dp, 0), wvb+vec2(dp, 0)) - dot(wvb-vec2(dp, 0), wvb-vec2(dp, 0))))/nr[TMOD]);
      atomicAdd(gr[ind[TMOD].x].y, int(lambda*0.5/3.0*(dot(wva+vec2( 0,dp), wva+vec2( 0,dp)) - dot(wva-vec2( 0,dp), wva-vec2( 0,dp))))/nr[TMOD]);
      atomicAdd(gr[ind[TMOD].x].y, int(lambda*0.5/3.0*(dot(wvb+vec2( 0,dp), wvb+vec2( 0,dp)) - dot(wvb-vec2( 0,dp), wvb-vec2( 0,dp))))/nr[TMOD]);

    }

    if (in_Position.y > 0){

      vec2 wva = p[ind[TMOD].y] - p[ind[TMOD].z]; //Distance from this Vertex to Prev
      vec2 wvb = p[ind[TMOD].y] - p[ind[TMOD].x]; //Distance from this Vertex to Next
      //Add Direct Energy Gradient
      atomicAdd(gr[ind[TMOD].y].x, int(lambda*0.5/3.0*(dot(wva+vec2(dp, 0), wva+vec2(dp, 0)) - dot(wva-vec2(dp, 0), wva-vec2(dp, 0))))/nr[TMOD]);
      atomicAdd(gr[ind[TMOD].y].x, int(lambda*0.5/3.0*(dot(wvb+vec2(dp, 0), wvb+vec2(dp, 0)) - dot(wvb-vec2(dp, 0), wvb-vec2(dp, 0))))/nr[TMOD]);
      atomicAdd(gr[ind[TMOD].y].y, int(lambda*0.5/3.0*(dot(wva+vec2( 0,dp), wva+vec2( 0,dp)) - dot(wva-vec2( 0,dp), wva-vec2( 0,dp))))/nr[TMOD]);
      atomicAdd(gr[ind[TMOD].y].y, int(lambda*0.5/3.0*(dot(wvb+vec2( 0,dp), wvb+vec2( 0,dp)) - dot(wvb-vec2( 0,dp), wvb-vec2( 0,dp))))/nr[TMOD]);

    }

    if (in_Position.y > 0){

      vec2 wva = p[ind[TMOD].z] - p[ind[TMOD].x]; //Distance from this Vertex to Prev
      vec2 wvb = p[ind[TMOD].z] - p[ind[TMOD].y]; //Distance from this Vertex to Next
      //Add Direct Energy Gradient
      atomicAdd(gr[ind[TMOD].z].x, int(lambda*0.5/3.0*(dot(wva+vec2(dp, 0), wva+vec2(dp, 0)) - dot(wva-vec2(dp, 0), wva-vec2(dp, 0))))/nr[TMOD]);
      atomicAdd(gr[ind[TMOD].z].x, int(lambda*0.5/3.0*(dot(wvb+vec2(dp, 0), wvb+vec2(dp, 0)) - dot(wvb-vec2(dp, 0), wvb-vec2(dp, 0))))/nr[TMOD]);
      atomicAdd(gr[ind[TMOD].z].y, int(lambda*0.5/3.0*(dot(wva+vec2( 0,dp), wva+vec2( 0,dp)) - dot(wva-vec2( 0,dp), wva-vec2( 0,dp))))/nr[TMOD]);
      atomicAdd(gr[ind[TMOD].z].y, int(lambda*0.5/3.0*(dot(wvb+vec2( 0,dp), wvb+vec2( 0,dp)) - dot(wvb-vec2( 0,dp), wvb-vec2( 0,dp))))/nr[TMOD]);

    }

  }

}
