#ifndef TPOSE_IO
#define TPOSE_IO

// io.h

#include <fstream>

namespace io {

using namespace std;
using namespace glm;

/*
================================================================================
                                Data Reading
================================================================================
*/

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

bool writeenergy( tri::triangulation* tr, string file ){

  cout<<"Exporting Energy to "<<file<<endl;
  ofstream out(file, ios::out);
  if(!out.is_open()){
    cout<<"Failed to open file "<<file<<endl;
    return false;
  }

  // Export Triangle Energy

  out<<"TENERGY"<<endl;
  for(size_t i = 0; i < tr->NT; i++)
    out<<tri::terr[i]<<endl;

  out<<"PENERGY"<<endl;
  for(size_t i = 0; i < tr->NP; i++)
    out<<tri::perr[i]<<endl;

  out<<"CN"<<endl;
  for(size_t i = 0; i < tr->NT; i++)
    out<<tri::cn[i]<<endl;

  out.close();

  return true;

}

}

#endif
