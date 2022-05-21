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
      A.push_back(pA/vec2(1200, 1200));
      B.push_back(pB/vec2(1200, 1200));
    }

  }

  return true;

}

}

#endif
