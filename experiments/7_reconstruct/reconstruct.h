/*
================================================================================
                    Reconstruct 3D Dataset from 2D Points
================================================================================
*/

#include <fstream>
#include <Eigen/Dense>
#include <Eigen/SVD>

using namespace std;
using namespace glm;
using namespace Eigen;

#include "math.h"

/*
================================================================================
                    Fundamental Matrix Estimation
================================================================================
*/

/*
========================================================================
                    Fundamental Matrix Estimation
========================================================================
*/

// Data-Point Normalization

Matrix3f Normalize( vector<vec2>& points ){

  vec2 c(0);
  for(auto& p: points)
    c += p;
  c /= (float)(points.size());

  float dist = 0.0f;
  for(auto& p: points){
    p -= c;
    dist += length(p);
  }
  dist /= (float)(points.size());

  float scale = sqrt(2) / dist;
  for(auto& p: points)
    p *= scale;

  Matrix3f H;
  H << scale,      0.0, -c.x*scale,
            0.0, scale, -c.y*scale,
            0.0,      0.0,       1.0;

  return H;

}

// Fundamental Matrix: Normalized 8-Point Algorithm

Matrix3f Fundamental( vector<vec2> pA, vector<vec2> pB ){

  cout<<"Computing Fundamental Matrix using Normalized 8-Point Algorithm"<<endl;

  // Make sure size is equal:

  Matrix3f F = Matrix3f::Identity();

  if(pA.size() != pB.size()){
    cout << "Matched sets have different size" << endl;
    return F;
  }

  if(pA.size() == 0){
    cout << "Matched sets are empty" << endl;
    return F;
  }

  // Normalize Points (Note: ALTERS THE POINTS)

  vector<vec2> poA = pA;
  vector<vec2> poB = pB;

  cout<<"Normalizing Data-Points..."<<endl;

  Matrix3f HA = Normalize(pA);
  Matrix3f HB = Normalize(pB);

  // Construct the Flattened Linear System

  cout<<"Constructing Linear System..."<<endl;

  size_t N = pA.size();
  MatrixXf A = MatrixXf::Zero(N, 9);

  for(size_t n = 0; n < N; n++){

    A(n, 0) = pA[n].x * pB[n].x;
    A(n, 1) = pA[n].y * pB[n].x;
    A(n, 2) = pB[n].x;
    A(n, 3) = pA[n].x * pB[n].y;
    A(n, 4) = pA[n].y * pB[n].y;
    A(n, 5) = pB[n].y;
    A(n, 6) = pA[n].x;
    A(n, 7) = pA[n].y;
    A(n, 8) = 1;

  }

  // Approximate the Solution Af = 0 by Least-Squares SVD

  cout<<"Solving Linear System..."<<endl;

  JacobiSVD<MatrixXf> AS(A, ComputeFullV);

  // Extract the Null-Space Vector as Approximation

  VectorXf f = AS.matrixV().col(8);

  // Fill 3x3 Matrix

  F <<  f(0), f(1), f(2),
        f(3), f(4), f(5),
        f(6), f(7), f(8);

  // Enforce the Rank-2 Constraint

  JacobiSVD<Matrix3f> FS(F, ComputeFullU | ComputeFullV);
  Vector3f S = FS.singularValues();

  cout<<"Smallest Singular Value of Estimate: "<<S(2)<<endl;

  cout<<"Enforcing Rank-2 Constraint and Unnormalizing..."<<endl;

  S(2) = 0; // Set smallest singular value to zero
  F = FS.matrixU() * S.asDiagonal() * FS.matrixV().transpose();

//  cout<<"RIGHT NULL-SPACE (EPIPOLE): "<<FS.matrixU().

  // Compute the Non-Normalized Fundamental Matrix

  //F = HB.transpose() * F * HA;
  F = HB.transpose() * F * HA;

  // Sample some error values...

  /*

  cout<<"Error Values: "<<endl;
  for(size_t n = 0; n < pA.size(); n++){
    Vector3f XA, XB;
    XA << poA[n].x, poA[n].y, 1.0f;
    XB << poB[n].x, poB[n].y, 1.0f;
    cout<< XB.transpose() * F * XA << endl;
  }

  */

  return F;

}

// Refine the Computation of F Iteratively:

vec2 Epipole( Matrix3f F, bool right = true ){

  JacobiSVD<Matrix3f> FS(F, ComputeFullU | ComputeFullV);

  if(right){
    Matrix3f V = FS.matrixV();
    Vector3f e = V.col(2);      //Last Column of V
    return vec2(e(0)/e(2), e(1)/e(2));
  }
  else{
    Matrix3f U = FS.matrixU();
    Vector3f e = U.col(2);      //Last Column of V
    return vec2(e(0)/e(2), e(1)/e(2));
  }

}

// Compute an Epipolar Line from the Fundamental Matrix

vec3 EpipolarLine( Matrix3f Fundamental, vec2 p ){
  return fromEigen3(Fundamental)*vec3(p.x, p.y, 1.0f);
}


/*
================================================================================
                                Data Reading
================================================================================
*/

bool read(string file, vector<vec2>& A, vector<vec2>& B){

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

  int n = 0;

  while(!in.eof()){

    getline(in, pstring);
    if(in.eof())
      break;

    n++;

  //  if(n < 15)
  //    continue;

  //  if(A.size() >= 15)
  //    break;

    int m = sscanf(pstring.c_str(), "%f %f %f %f", &pA.x, &pA.y, &pB.x, &pB.y);
    if(m == 4){
      A.push_back(pA);
      B.push_back(pB);
    }

  }

  return true;

}
