/*
================================================================================
                                  t-pose: io.h
================================================================================

This header file provides an interface for data io from files for various data.

*/

#ifndef TPOSE_IO
#define TPOSE_IO

#include <boost/filesystem.hpp>
#include <fstream>

namespace tpose {
namespace io {

using namespace std;
using namespace glm;

bool readmatches(string file, vector<vec2>& A, vector<vec2>& B){

  // Load the Points

  cout<<"Importing from file "<<file<<endl;
  ifstream in(file, ios::in);
  if(!in.is_open()){
    cout<<"Failed to open file "<<file<<endl;
    return false;
  }

  string pstring;

  vec2 pA = vec2(0);
  vec2 pB = vec2(0);

  while(!in.eof()){

    getline(in, pstring);
    if(in.eof())
      break;

    int m = sscanf(pstring.c_str(), "%f %f %f %f", &pA.x, &pA.y, &pB.x, &pB.y);
    if(m == 4){
      A.push_back(pA);
      B.push_back(pB);
    }

  }

  return true;

}

/*
================================================================================
                                Data Writing
================================================================================
*/

bool writeenergy( tpose::triangulation* tr, string file ){

  cout<<"Exporting Energy to "<<file<<endl;
  ofstream out(file, ios::out);
  if(!out.is_open()){
    cout<<"Failed to open file "<<file<<endl;
    return false;
  }

  // Export Triangle Energy

  out<<"TENERGY"<<endl;
  for(size_t i = 0; i < tr->NT; i++)
    out<<tpose::terr[i]<<endl;

  out<<"PENERGY"<<endl;
  for(size_t i = 0; i < tr->NP; i++)
    out<<tpose::perr[i]<<endl;

  out<<"CN"<<endl;
  for(size_t i = 0; i < tr->NT; i++)
    out<<tpose::cn[i]<<endl;

  out.close();

  return true;

}




/*
================================================================================
                            Triangulation IO
================================================================================

This defines a binary triangulation format which is stackable!

*/

#ifdef TPOSE_TRIANGULATION

bool read( tpose::triangulation* tri, string file, bool dowarp ){

  cout<<"Importing triangulation from "<<file<<" ... ";

  if(!tri->in.is_open()){
    tri->in.open( file, ios::binary | ios::in );
    if(!tri->in.is_open()){
      cout<<"failed to open file."<<endl;
      exit(0);
    }
  }

  if(tri->in.eof()){
    tri->in.close();
    cout<<"end of file."<<endl;
    return false;
  }

  // Top-Level Metadata

  tri->in.read( (char*)( &tpose::RATIO ), sizeof( float ));

  // Per-Triangle Data

  tri->in.read( (char*)( &tri->NT ), sizeof( int ));

  tri->triangles.resize(tri->NT);
  tri->halfedges.resize(3*tri->NT);
  tri->colors.resize(tri->NT);

  for(size_t t = 0; t < tri->NT; t++){

    tri->in.read( (char*)( &tri->triangles[t][0] ), sizeof( int ));
    tri->in.read( (char*)( &tri->triangles[t][1] ), sizeof( int ));
    tri->in.read( (char*)( &tri->triangles[t][2] ), sizeof( int ));
    tri->triangles[t].w = 0.0f;

    tri->in.read( (char*)( &tri->halfedges[3*t+0] ), sizeof( int ));
    tri->in.read( (char*)( &tri->halfedges[3*t+1] ), sizeof( int ));
    tri->in.read( (char*)( &tri->halfedges[3*t+2] ), sizeof( int ));

    tri->in.read( (char*)( &tri->colors[t][0] ), sizeof( int ));
    tri->in.read( (char*)( &tri->colors[t][1] ), sizeof( int ));
    tri->in.read( (char*)( &tri->colors[t][2] ), sizeof( int ));
    tri->colors[t].w = 1.0f;

  }

  // Per-Vertex Data

  tri->in.read( (char*)( &tri->NP ), sizeof( int ));

  tri->points.resize(tri->NP);
  tri->originpoints.resize(tri->NP);

  for(size_t p = 0; p < tri->NP; p++){

    tri->in.read( (char*)( &tri->points[p][0] ), sizeof( float ));
    tri->in.read( (char*)( &tri->points[p][1] ), sizeof( float ));

    tri->in.read( (char*)( &tri->originpoints[p][0] ), sizeof( float ));
    tri->in.read( (char*)( &tri->originpoints[p][1] ), sizeof( float ));

  }

  cout<<"success."<<endl;
  return true;

}

void write( tpose::triangulation* tri, string file, bool normalize = true ){

  if(!tri->out.is_open()){
    tri->out.open( file, ios::binary | ios::out );
    if(!tri->out.is_open()){
      cout<<"Failed to open file "<<file<<endl;
      exit(0);
    }
  }

  cout<<"Exporting to "<<file<<endl;

  // Top-Level Metadata

  tri->out.write( reinterpret_cast<const char*>( &tpose::RATIO ), sizeof( float ));

  // Per-Triangle Data

  tri->out.write( reinterpret_cast<const char*>( &tri->NT ), sizeof( int ));

  for(size_t t = 0; t < tri->NT; t++){

    tri->out.write( reinterpret_cast<const char*>( &tri->triangles[t][0] ), sizeof( int ));
    tri->out.write( reinterpret_cast<const char*>( &tri->triangles[t][1] ), sizeof( int ));
    tri->out.write( reinterpret_cast<const char*>( &tri->triangles[t][2] ), sizeof( int ));

    tri->out.write( reinterpret_cast<const char*>( &tri->halfedges[3*t+0] ), sizeof( int ));
    tri->out.write( reinterpret_cast<const char*>( &tri->halfedges[3*t+1] ), sizeof( int ));
    tri->out.write( reinterpret_cast<const char*>( &tri->halfedges[3*t+2] ), sizeof( int ));

    tri->out.write( reinterpret_cast<const char*>( &tri->colors[t][0] ), sizeof( int ));
    tri->out.write( reinterpret_cast<const char*>( &tri->colors[t][1] ), sizeof( int ));
    tri->out.write( reinterpret_cast<const char*>( &tri->colors[t][2] ), sizeof( int ));

  }

  // Per-Vertex Data

  tri->out.write( reinterpret_cast<const char*>( &tri->NP ), sizeof( int ));

  for(size_t p = 0; p < tri->NP; p++){

    tri->out.write( reinterpret_cast<const char*>( &tri->points[p][0] ), sizeof( float ));
    tri->out.write( reinterpret_cast<const char*>( &tri->points[p][1] ), sizeof( float ));

    tri->out.write( reinterpret_cast<const char*>( &tri->originpoints[p][0] ), sizeof( float ));
    tri->out.write( reinterpret_cast<const char*>( &tri->originpoints[p][1] ), sizeof( float ));

  }

}

#endif

};  // End of Namespace io
};  // End of Namespace tpose

#endif
