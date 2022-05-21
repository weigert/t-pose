/*
================================================================================
                    Reconstruct 3D Dataset from 2D Points
================================================================================
*/

#include <fstream>
#include <Eigen/Dense>
#include <Eigen/Eigenvalues>

using namespace std;
using namespace glm;
using namespace Eigen;

#include "math.h"

/*
================================================================================
                    Fundamental Matrix Estimation
================================================================================

New Approach:

Ransac(
- Normalized 8-Point Initial Guess
  - Sampson Optimize
)

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

  // Compute the Non-Normalized Fundamental Matrix

  F = HB.transpose() * F * HA;

  // Sample some error values...

  F /= F(2,2);
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
  vec3 l = fromEigen3(Fundamental)*vec3(p.x, p.y, 1.0f);
  return (l/l.z);
}


Matrix3f FundamentalSAMPSON( vector<vec2> pA, vector<vec2> pB ){

  cout<<"Computing Fundamental Matrix using Sampson Cost Function Iteration"<<endl;

  Matrix3f F = Fundamental(pA, pB);     // Initial Fundamental Matrix Guess
  size_t N = pA.size();                 // Size of System

  MatrixXf A = MatrixXf::Zero(N, 9);    // Solution Matrix
  VectorXf W = VectorXf(N);             // Contribution Weights

  // Normalize the Points

  Matrix3f HA = Normalize(pA);
  Matrix3f HB = Normalize(pB);

  // Iterate

  for(size_t k = 0; k < 100; k++){

    // Compute Weights

    for(size_t n = 0; n < N; n++){

      vec3 L = EpipolarLine(F.transpose(), pB[n]);
      vec3 R = EpipolarLine(F, pA[n]);
      W(n) = 1.0f / ( L.x*L.x + L.y*L.y + R.x*R.x + R.y*R.y );

    }

    for(size_t n = 0; n < N; n++){

      A(n, 0) = W[n] * pA[n].x * pB[n].x;
      A(n, 1) = W[n] * pA[n].y * pB[n].x;
      A(n, 2) = W[n] * pB[n].x;
      A(n, 3) = W[n] * pA[n].x * pB[n].y;
      A(n, 4) = W[n] * pA[n].y * pB[n].y;
      A(n, 5) = W[n] * pB[n].y;
      A(n, 6) = W[n] * pA[n].x;
      A(n, 7) = W[n] * pA[n].y;
      A(n, 8) = W[n] * 1;

    }

    JacobiSVD<MatrixXf> AS(A, ComputeFullV);
    VectorXf f = AS.matrixV().col(8);

    // Fill 3x3 Matrix

    F <<  f(0), f(1), f(2),
          f(3), f(4), f(5),
          f(6), f(7), f(8);

    JacobiSVD<Matrix3f> FS(F, ComputeFullU | ComputeFullV);
    Vector3f S = FS.singularValues();

    cout<<"Smallest Singular Value of Estimate: "<<S(2)<<endl;

    S(2) = 0; // Set smallest singular value to zero
    F = FS.matrixU() * S.asDiagonal() * FS.matrixV().transpose();

  }



  // Compute the Non-Normalized Fundamental Matrix

  F = HB.transpose() * F * HA;

  // Sample some error values...

  F /= F(2,2);
  return F;

}


// Ransac F

Matrix3f FundamentalRANSAC( vector<vec2>& pA, vector<vec2>& pB ){

  // Generate Sub-Samples
  // Generate "Best-Guess F", Compute Inliers
  // Maximum Inliners is the Matrix

  // Generate Random Samples

  MatrixXf F; //Best Guess
  int maxinliers = 0;

  for(size_t i = 0; i < 10000; i++){

    vector<vec2> poA;
    vector<vec2> poB;

    for(size_t n = 0; n < 8; n++){

      poA.push_back(pA[rand()%pA.size()]);
      poB.push_back(pB[rand()%pB.size()]);

      MatrixXf TF = FundamentalSAMPSON(poA, poB);
      int curinliers = 0;

      // Compute the number of inliners
      for(size_t p = 0; p < pA.size(); p++){

        Vector3f xA, xB;
        xA << pA[p].x, pA[p].y, 1;
        xB << pB[p].x, pB[p].y, 1;

        if(abs((xB.transpose()*TF*xA)(0)) < 1E-2)
          curinliers++;

      }

      if(curinliers > maxinliers){
        F = TF;
        maxinliers = curinliers;
      }

    }

  }

  cout<<"inliers: "<<maxinliers<<endl;



  // Recompute F for the set of Inliners

  vector<vec2> poA;
  vector<vec2> poB;

  for(size_t p = 0; p < pA.size(); p++){

    Vector3f xA, xB;
    xA << pA[p].x, pA[p].y, 1;
    xB << pB[p].x, pB[p].y, 1;

    if(abs((xB.transpose()*F*xA)(0)) < 1E-2){
      poA.push_back(pA[p]);
      poB.push_back(pB[p]);
    }

  }

  F = FundamentalSAMPSON(poA, poB);

  /*

  vector<vec2> poA;
  vector<vec2> poB;

  for(size_t p = 0; p < pA.size(); p++){

    Vector3f xA, xB;
    xA << pA[p].x, pA[p].y, 1;
    xB << pB[p].x, pB[p].y, 1;

    if(abs((xB.transpose()*F*xA)(0)) < 1E-2){
      poA.push_back(pA[p]);
      poB.push_back(pB[p]);
    }

  }

  // Only return the inliers for the sampson estimation

  pA = poA;
  pB = poB;

  */

  return F;

}




/*
================================================================================
                                Triangulation
================================================================================

1. Solve the 6-Order Polynomial for the Approximate Points
2. Triangulate using the Direct Homogeneous Method

*/

// Root-Calculator

/*

template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

template<typename T>
vector<T> bisect(function<T(T)> f, T min = -1000.0f, T max = 1000.0f){

  vector<T> roots;

  const size_t MAXITER = 2000;
  size_t NITER = 0;
  const float tol = 1E-6;

  while(NITER++ < MAXITER){

      T cur = (max + min)/2;
      if(f(c) < )

  }

  if(f(min)*f(max) < 0)

}

*/

double horner(double x, vector<double> a){

  double result = a[a.size()-1];
  for(int i = a.size()-2; i >= 0; i--)
    result = result*x + a[i];
  return result;

}

VectorXcd roots(vector<double> a){

  const size_t K = a.size();
  MatrixXd C = MatrixXd::Zero(K,K);

  for(size_t k = 0; k < K; k++)
    C(k,K-1) = -a[k];

  for(size_t k = 1; k < K; k++)
    C(k,k-1) = 1;

//  cout<<"Companion Matrix: "<<C<<endl;

  EigenSolver<MatrixXd> CS(C);
//  cout<<"ROOTS: "<<CS.eigenvalues()<<endl;

  return CS.eigenvalues();

}

vector<double> realroots(vector<double> a, vector<double> b){

  vector<double> rR;

  VectorXcd R = roots(a);
  for(size_t r = 0; r < a.size(); r++)
    if(R(r).imag() == 0) rR.push_back(R(r).real());

  // Refine the Estimate... (Newton Method)

  for(auto& r: rR)
  for(size_t n = 0; n < 25; n++)
    r -= horner(r, b)/horner(r, {b[1]*1.0, b[2]*2.0, b[3]*3.0, b[4]*4.0, b[5]*5.0, b[6]*6.0});

  return rR;

}

// Wrapper

void triangulate(MatrixXf F, vec2& A, vec2& B){

  // Matrices to Transform to Center

  Matrix3f TA, TB;

  TA << 1, 0, -A.x,
        0, 1, -A.y,
        0, 0,    1;

  TB << 1, 0, -B.x,
        0, 1, -B.y,
        0, 0,    1;

  // Shift F

  MatrixXf dF = TB.transpose().inverse() * F * TA.inverse();

  // Compute Left and Right Epipoles, Normalize

  JacobiSVD<Matrix3f> FS(dF, ComputeFullU | ComputeFullV);
  Matrix3f V = FS.matrixV();
  Matrix3f U = FS.matrixU();

  Vector3f eA = V.col(2); //Right Epipole
  eA /= sqrt(eA(0)*eA(0) + eA(1)*eA(1));

  Vector3f eB = U.col(2); //Left Epipole
  eB /= sqrt(eB(0)*eB(0) + eB(1)*eB(1));

  // Compute Rotation Matrices

  Matrix3f RA, RB;

  RA << eA(0), eA(1), 0,
       -eA(1), eA(0), 0,
            0,     0, 1;

  RB << eB(0), eB(1), 0,
       -eB(1), eB(0), 0,
            0,     0, 1;

  // Rotate F

  dF = RB * dF * RA.transpose();

  //cout<<"Fundamental Matrix F "<<F<<endl;

  // Polynomial Coefficients

  double m = eA(2);
  double n = eB(2);
  double a = dF(1, 1);
  double b = dF(1, 2);
  double c = dF(2, 1);
  double d = dF(2, 2);

  // Solve for the Roots of this Polynomial (Gradient of Cost-Function)

  //function<double(double)> g = [&](double t){
  //  return t*pow( pow( a*t+b , 2 ) + n*n * pow( c*t+d , 2 ) , 2 ) - (a*d-b*c) * pow( 1.0 + m*m*t*t, 2 ) *( a*t + b ) * ( c*t + d );
  //};

  function<double(double)> g = [&](double t){
    return t * ((a*t+b)*(a*t+b) + n*n * (c*t+d)*(c*t+d)) * ((a*t+b)*(a*t+b) + n*n * (c*t+d)*(c*t+d)) - (a*d-b*c) * (1.0 + m*m*t*t) * (1.0 + m*m*t*t) *( a*t + b ) * ( c*t + d );
  };

  double a0 = b*b*c*d - a*b*d*d;

  double a1 = b*b*b*b + (b*b*c*c - a*a*d*d) + 2.0*b*b*d*d*n*n + d*d*d*d*n*n*n*n;

  double a2 = (a*b*c*c - a*a*c*d) + 4.0*a*b*b*b + 2.0*(b*b*c*d - a*b*d*d)*m*m + 4.0*(a*b*d*d + b*b*c*d)*n*n + 4.0*c*d*d*d*n*n*n*n;

  double a3 = 6.0*a*a*b*b + 2.0*(b*b*c*c - a*a*d*d)*m*m + 2.0*a*a*d*d*n*n + 8.0*a*b*c*d*n*n + 2.0*b*b*c*c*n*n + 6.0*c*c*d*d*n*n*n*n;

  double a4 = (b*b*c*d - a*b*d*d)*m*m*m*m + 4.0*a*a*a*b + 2.0*(a*b*c*c - a*a*c*d)*m*m + 4.0*(a*a*c*d + a*b*c*c)*n*n  + 4.0*c*c*c*d*n*n*n*n;

  double a5 = a*a*a*a + (b*b*c*c - a*a*d*d)*m*m*m*m + 2.0*a*a*c*c*n*n + c*c*c*c*n*n*n*n;

  double a6 = (a*b*c*c - a*a*c*d)*m*m*m*m;

  vector<double> R = realroots({a0/a6, a1/a6, a2/a6, a3/a6, a4/a6, a5/a6}, {a0, a1, a2, a3, a4, a5, a6});


  // Find the Lowest Cost-Function Value

  function<double(double)> S = [&](double t){
    return t*t/(1.0f + m*m*t*t) + pow(c*t+d,2)/(pow(a*t+b,2) + n*n*pow(c*t+d,2));
  };

  //cout<<"REAL ROOTS VALUE: "<<endl;
  //for(auto& r: R){
//    cout<<horner(r, {a0, a1, a2, a3, a4, a5, a6})<<endl;
  //  cout<<g(r)<<endl;
  //}

  //cout<<g(1.0)-horner(1.0, {a0, a1, a2, a3, a4, a5, a6})<<endl;
  //cout<<g(1.0)<<" "<<a0+a1+a2+a3+a4+a5+a6<<endl;
//horner(1, {a0/a6, a1/a6, a2/a6, a3/a6, a4/a6, a5/a6})


  int minind = 0;
  double minerr = S(R[0]);
  //cout<<"Err: "<<minerr<<endl;
  for(size_t r = 1; r < R.size(); r++){
    double curerr = S(r);
  //  cout<<"Err: "<<curerr<<endl;
    if(curerr < minerr){
      minind = r;
      minerr = curerr;
    }
  }

//  cout<<R.size()<<endl;

  cout<<"Smallest Error: ("<<minind<<") "<<minerr<<endl;
  double t = R[minind];

  // Evaluate the Lines

  Vector3f LA, LB;
  LA << t*m, 1, -t;
  LB << -n*(c*t+d), a*t+b, c*t+d;


  // Compute Approximate Points
  Vector3f XA, XB;

  XA << -LA(0)*LA(2), -LA(1)*LA(2), LA(0)*LA(0) + LA(1)*LA(1);
  XB << -LB(0)*LB(2), -LB(1)*LB(2), LB(0)*LB(0) + LB(1)*LB(1);

  XA /= XA(2);
  XB /= XB(2);

//  cout<<XA<<endl;
//  cout<<XB<<endl;

  // Transform Back

//  cout<<XA<<endl;
//  cout<<XB.transpose()*dF*XA<<endl;


  //cout<<XB.transpose()*dF*XA<<endl;

  XA = TA.inverse()*RA.transpose()*XA;
  XB = TB.inverse()*RB.transpose()*XB;



  // Matched Points

  Vector3f xA, xB;
  xA << A.x, A.y, 1;
  xB << B.x, B.y, 1;

  xA /= xA(2);
  xB /= xB(2);

  //cout<<"NEAREST POINTS:"<<endl;
  //cout<<xA<<endl;
  //cout<<XA<<endl;
  //cout<<xB<<endl;
  //cout<<XB<<endl;

  cout<<(xA - XA)<<endl;
  cout<<(xB - XB)<<endl;

  //cout<<XB.transpose()*dF*XA<<endl;

  // Shift the Point to the Projection Estimate

  A = vec2(XA(0), XA(1));
  B = vec2(XB(0), XB(1));

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
      A.push_back(pA/vec2(1200, 675));
      B.push_back(pB/vec2(1200, 675));
    }

  }

  return true;

}
