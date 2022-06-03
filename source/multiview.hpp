/*
================================================================================
													t-pose: multiview.hpp
================================================================================

This header defines a number of functions to help in computing fundamental matrices,
epipolar geometries and perform point triangulation for multiview geometry.

*/

#ifndef TPOSE_MULTIVIEW
#define TPOSE_MULTIVIEW

// unproject.h

#include <Eigen/Dense>
#include <Eigen/Eigenvalues>

#include <opencv2/opencv.hpp>
#include <opencv2/core/eigen.hpp>

#include "math.h"
#include "utility.h"

namespace tpose{
namespace mview {

using namespace std;
using namespace glm;
using namespace Eigen;

/*
================================================================================
                          Camera Intrinsic Matrix
================================================================================
*/

int check = 3;
float px = 488.421 / 960.0f;
float py = 268.8 / 960.0f;
float fx = 673.101 / 960.0f;
float fy = 673.328 / 960.0f;

// 488.421 268.8 673.101 673.328 0.142736 -0.466688 -0.000759649 0.000336397 0.420803


Matrix3f Camera(){

  MatrixXf K = MatrixXf(3, 3);
  K <<  1.0f/fx, 0, px,
        0, 1.0f/fy, py,
        0, 0, 1;
  return K;

}

/*
================================================================================
                      Data Normalization and Epipoles
================================================================================
*/

// Data-Point Normalization

Matrix3f normalize( vector<vec2>& points ){

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

// Epipoles and Epipolar Lines

vec2 epole( Matrix3f F, bool right = true ){

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

vec3 eline( Matrix3f F, vec2 p ){

  vec3 l = fromEigen3(F)*vec3(p.x, p.y, 1.0f);
  return (l/l.z);

}

vec3 eline( vec2 p, Matrix3f F ){

  vec3 l = fromEigen3(F.transpose())*vec3(p.x, p.y, 1.0f);
  return (l/l.z);

}

/*
================================================================================
                      Fundamental Matrix Estimation
================================================================================
*/

// Normalized 8-Point Algorithm

Matrix3f F_8Point( vector<vec2> pA, vector<vec2> pB ){

  Matrix3f F = Matrix3f::Identity();

  if(pA.size() != pB.size()){
    cout << "Error: Matched sets have different size" << endl;
    return F;
  }

  if(pA.size() == 0){
    cout << "Error: Matched sets are empty" << endl;
    return F;
  }

  Matrix3f HA = normalize(pA);        //Normalize Data A
  Matrix3f HB = normalize(pB);        //Normalize Data B

  size_t N = pA.size();
  MatrixXf A = MatrixXf::Zero(N, 9);  //Linear System

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

  JacobiSVD<MatrixXf> AS(A, ComputeFullV);  //Least-Squares SVD
  VectorXf f = AS.matrixV().col(8);         //Solution in Right Null-Space

  F <<  f(0), f(1), f(2),                   //Unflatten Matrix
        f(3), f(4), f(5),
        f(6), f(7), f(8);

  // Enforce Rank-2 Constraint

  JacobiSVD<Matrix3f> FS(F, ComputeFullU | ComputeFullV);
  Vector3f S = FS.singularValues();
  S(2) = 0;
  F = FS.matrixU() * S.asDiagonal() * FS.matrixV().transpose();

  F = HB.transpose() * F * HA;              //Unnormalize
  F /= F(2,2);                              //Scale

  return F;

}

// Iterative Sampson Distance Minimizer (First-Order Geometric Reprojection Cost)

Matrix3f F_Sampson( vector<vec2> pA, vector<vec2> pB ){

  Matrix3f F = F_8Point(pA, pB);        // Initial Fundamental Matrix Guess
  size_t N = pA.size();

  MatrixXf A = MatrixXf::Zero(N, 9);    // Solution Matrix
  VectorXf W = VectorXf(N);             // Contribution Weights

  Matrix3f HA = normalize(pA);
  Matrix3f HB = normalize(pB);

  const size_t MAXITER = 100;
  for(size_t k = 0; k < MAXITER; k++){

    for(size_t n = 0; n < N; n++){

      vec3 L = eline(F.transpose(), pB[n]);
      vec3 R = eline(F, pA[n]);
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

    F <<  f(0), f(1), f(2),
          f(3), f(4), f(5),
          f(6), f(7), f(8);

    JacobiSVD<Matrix3f> FS(F, ComputeFullU | ComputeFullV);
    Vector3f S = FS.singularValues();
    S(2) = 0;
    F = FS.matrixU() * S.asDiagonal() * FS.matrixV().transpose();

  }

  F = HB.transpose() * F * HA;
  F /= F(2,2);

  return F;

}

Matrix3f F_Sampson( vector<vec2> pA, vector<vec2> pB, vector<float> w ){

  Matrix3f F = F_8Point(pA, pB);        // Initial Fundamental Matrix Guess
  size_t N = pA.size();

  MatrixXf A = MatrixXf::Zero(N, 9);    // Solution Matrix
  VectorXf W = VectorXf(N);             // Contribution Weights

  Matrix3f HA = normalize(pA);
  Matrix3f HB = normalize(pB);

  const size_t MAXITER = 100;
  for(size_t k = 0; k < MAXITER; k++){

    for(size_t n = 0; n < N; n++){

      vec3 L = eline(F.transpose(), pB[n]);
      vec3 R = eline(F, pA[n]);
      W(n) = 1.0f / ( L.x*L.x + L.y*L.y + R.x*R.x + R.y*R.y );

    }

    for(size_t n = 0; n < N; n++){

      A(n, 0) = w[n] * W[n] * pA[n].x * pB[n].x;
      A(n, 1) = w[n] * W[n] * pA[n].y * pB[n].x;
      A(n, 2) = w[n] * W[n] * pB[n].x;
      A(n, 3) = w[n] * W[n] * pA[n].x * pB[n].y;
      A(n, 4) = w[n] * W[n] * pA[n].y * pB[n].y;
      A(n, 5) = w[n] * W[n] * pB[n].y;
      A(n, 6) = w[n] * W[n] * pA[n].x;
      A(n, 7) = w[n] * W[n] * pA[n].y;
      A(n, 8) = w[n] * W[n] * 1;

    }

    JacobiSVD<MatrixXf> AS(A, ComputeFullV);
    VectorXf f = AS.matrixV().col(8);

    F <<  f(0), f(1), f(2),
          f(3), f(4), f(5),
          f(6), f(7), f(8);

    JacobiSVD<Matrix3f> FS(F, ComputeFullU | ComputeFullV);
    Vector3f S = FS.singularValues();
    S(2) = 0;
    F = FS.matrixU() * S.asDiagonal() * FS.matrixV().transpose();

  }

  F = HB.transpose() * F * HA;
  F /= F(2,2);

  return F;

}

// OpenCV Implementation of LMEDS (for now)

Matrix3f F_LMEDS( vector<vec2> A, vector<vec2> B ){

  Matrix3f F;

  vector<cv::Point2f> pA, pB;
  for(size_t i = 0; i < A.size(); i++){

    // No Boundary Points

    if(A[i].x <= -tri::RATIO) continue;
    if(A[i].x >= tri::RATIO) continue;
    if(A[i].y <= -1) continue;
    if(A[i].y >= 1) continue;
    if(B[i].x <= -tri::RATIO) continue;
    if(B[i].x >= tri::RATIO) continue;
    if(B[i].y <= -1) continue;
    if(B[i].y >= 1) continue;

    pA.emplace_back(A[i].x, A[i].y);
    pB.emplace_back(B[i].x, B[i].y);

  }

  cv::Mat cvF = cv::findFundamentalMat(pA, pB, cv::FM_RANSAC, 0.0025, 0.99);
  cv2eigen(cvF, F);
  return F;

}

Matrix3f F_RANSAC( vector<vec2> A, vector<vec2> B ){

  Matrix3f F;

  vector<cv::Point2f> pA, pB;
  for(size_t i = 0; i < A.size(); i++){

    // No Boundary Points

    if(A[i].x <= -tri::RATIO) continue;
    if(A[i].x >= tri::RATIO) continue;
    if(A[i].y <= -1) continue;
    if(A[i].y >= 1) continue;
    if(B[i].x <= -tri::RATIO) continue;
    if(B[i].x >= tri::RATIO) continue;
    if(B[i].y <= -1) continue;
    if(B[i].y >= 1) continue;

    pA.emplace_back(A[i].x, A[i].y);
    pB.emplace_back(B[i].x, B[i].y);

  }

  cv::Mat cvF = cv::findFundamentalMat(pA, pB, cv::FM_RANSAC, 0.001, 0.99);
  cv2eigen(cvF, F);
  return F;

}

/*
================================================================================
                                Triangulation
================================================================================
*/

// Homogeneous Direct Linear Transform
// Solves for X, if xA = PA*X, xB = PB*X

Vector4f HDLT(MatrixXf PA, MatrixXf PB, Vector3f xA, Vector3f xB){

  MatrixXf H(4, 4);
  H.row(0) = xA(0)*PA.row(2) - PA.row(0);
  H.row(1) = xA(1)*PA.row(2) - PA.row(1);
  H.row(2) = xB(0)*PB.row(2) - PB.row(0);
  H.row(3) = xB(1)*PB.row(2) - PB.row(1);
  JacobiSVD<MatrixXf> HS(H, ComputeFullV);
  return HS.matrixV().col(3);

}


// Pose Matrix

struct Pose {
  Matrix3f R1;
  Matrix3f R2;
  Vector3f t;
};

Pose GetPose( Matrix3f Essential ){

  Matrix3f W;
  W <<  0,-1, 0,
        1, 0, 0,
        0, 0, 1;

  JacobiSVD<Matrix3f> ES(Essential, ComputeFullU | ComputeFullV);
  Matrix3f U = ES.matrixU();
  Matrix3f V = ES.matrixV();

  // Guess for t: Note that sign is not determined
  // Guess for R: One of the two is not valid!

  Pose pose;
  pose.t = U.col(2);
  pose.R1 = U*W*V.transpose();
  pose.R2 = U*W.transpose()*V.transpose();
  return pose;

};

// Wrapper

void triangulate(MatrixXf F, vec2& A, vec2& B){

  // Transform F to Points (i.e.: points to origin)

  Matrix3f TA, TB;

  TA << 1, 0, -A.x,
        0, 1, -A.y,
        0, 0,    1;

  TB << 1, 0, -B.x,
        0, 1, -B.y,
        0, 0,    1;

  F = TB.transpose().inverse() * F * TA.inverse();

  // Compute Left and Right Epipoles, Normalize

  JacobiSVD<Matrix3f> FS(F, ComputeFullU | ComputeFullV);
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

  F = RB * F * RA.transpose();

  // Parameters

  double m = eA(2);
  double n = eB(2);
  double a = F(1, 1);
  double b = F(1, 2);
  double c = F(2, 1);
  double d = F(2, 2);

  // Geometric Cost-Function and Gradient (6-Degree Polynomials)

  const function<double(double)> S = [&](double t){
    return t*t/(1.0f + m*m*t*t) + pow(c*t+d,2)/(pow(a*t+b,2) + n*n*pow(c*t+d,2));
  };

  const function<double(double)> G = [&](double t){
    return t * ((a*t+b)*(a*t+b) + n*n * (c*t+d)*(c*t+d)) * ((a*t+b)*(a*t+b) + n*n * (c*t+d)*(c*t+d)) - (a*d-b*c) * (1.0 + m*m*t*t) * (1.0 + m*m*t*t) *( a*t + b ) * ( c*t + d );
  };

  // Polynomial Coefficients (Gradient Polynomial)

  double a0 = b*b*c*d - a*b*d*d;
  double a1 = b*b*b*b + (b*b*c*c - a*a*d*d) + 2.0*b*b*d*d*n*n + d*d*d*d*n*n*n*n;
  double a2 = (a*b*c*c - a*a*c*d) + 4.0*a*b*b*b + 2.0*(b*b*c*d - a*b*d*d)*m*m + 4.0*(a*b*d*d + b*b*c*d)*n*n + 4.0*c*d*d*d*n*n*n*n;
  double a3 = 6.0*a*a*b*b + 2.0*(b*b*c*c - a*a*d*d)*m*m + 2.0*a*a*d*d*n*n + 8.0*a*b*c*d*n*n + 2.0*b*b*c*c*n*n + 6.0*c*c*d*d*n*n*n*n;
  double a4 = (b*b*c*d - a*b*d*d)*m*m*m*m + 4.0*a*a*a*b + 2.0*(a*b*c*c - a*a*c*d)*m*m + 4.0*(a*a*c*d + a*b*c*c)*n*n  + 4.0*c*c*c*d*n*n*n*n;
  double a5 = a*a*a*a + (b*b*c*c - a*a*d*d)*m*m*m*m + 2.0*a*a*c*c*n*n + c*c*c*c*n*n*n*n;
  double a6 = (a*b*c*c - a*a*c*d)*m*m*m*m;

  // Solve for Roots of Gradient Polynomial

  vector<double> R = realroots({a0, a1, a2, a3, a4, a5, a6});

  // Find Global Cost Minimum

  double t = R[0];
  double minerr = S(R[0]);

  for(size_t r = 1; r < R.size(); r++){
    double curerr = S(r);
    if(curerr < minerr){
      t = R[r];
      minerr = curerr;
    }
  }

  // Compute Approximate Points

  Vector3f LA, LB;
  Vector3f XA, XB;

  LA << t*m, 1, -t;
  LB << -n*(c*t+d), a*t+b, c*t+d;

  XA << -LA(0)*LA(2), -LA(1)*LA(2), LA(0)*LA(0) + LA(1)*LA(1);
  XB << -LB(0)*LB(2), -LB(1)*LB(2), LB(0)*LB(0) + LB(1)*LB(1);

  // Transform Back

  XA = TA.inverse()*RA.transpose()*XA;
  XB = TB.inverse()*RB.transpose()*XB;

  XA /= XA(2);
  XB /= XB(2);

  // Shift the Point to the Projection Estimate

  A = vec2(XA(0), XA(1));
  B = vec2(XB(0), XB(1));

}

vector<vec4> triangulate(MatrixXf F, MatrixXf K, vector<vec2> A, vector<vec2> B){

  vector<vec4> points;

  // Shift Points to their Optimal Positions

  const size_t N = A.size();
  for(size_t n = 0; n < N; n++)
    triangulate(F, A[n], B[n]);

  // Compute the Essential Matrix

  Matrix3f E = K.transpose()*F*K;

  // Output E

  JacobiSVD<Matrix3f> FS(F);
  cout<<"F Singular Values: "<<FS.singularValues()<<endl;


  JacobiSVD<Matrix3f> ES(E);
  cout<<"E Singular Values: "<<ES.singularValues()<<endl;

  // Compute Pose

  MatrixXf PA = MatrixXf::Zero(3, 4);
  MatrixXf PB = MatrixXf::Zero(3, 4);

  PA << 1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0;

  Pose P = GetPose(E);

  cout<<P.R1<<endl;
  cout<<P.R2<<endl;
  cout<<P.t<<endl;

  function<bool(int)> docheck = [&](int k){

    if(k == 0){
      PB << P.R1(0,0),  P.R1(0,1),  P.R1(0,2), P.t(0),
            P.R1(1,0),  P.R1(1,1),  P.R1(1,2), P.t(1),
            P.R1(2,0),  P.R1(2,1),  P.R1(2,2), P.t(2);
    }
    if(k == 1){
      PB << P.R1(0,0),  P.R1(0,1),  P.R1(0,2), -P.t(0),
            P.R1(1,0),  P.R1(1,1),  P.R1(1,2), -P.t(1),
            P.R1(2,0),  P.R1(2,1),  P.R1(2,2), -P.t(2);
    }
    if(k == 2){
      PB << P.R2(0,0),  P.R2(0,1),  P.R2(0,2), P.t(0),
            P.R2(1,0),  P.R2(1,1),  P.R2(1,2), P.t(1),
            P.R2(2,0),  P.R2(2,1),  P.R2(2,2), P.t(2);
    }
    if(k == 3){
      PB << P.R2(0,0),  P.R2(0,1),  P.R2(0,2), -P.t(0),
            P.R2(1,0),  P.R2(1,1),  P.R2(1,2), -P.t(1),
            P.R2(2,0),  P.R2(2,1),  P.R2(2,2), -P.t(2);
    }

    Vector3f XA, XB;
    XA << A[0].x, A[0].y, 1;
    XB << B[0].x, B[0].y, 1;

    Vector4f X = HDLT(PA, PB, XA, XB);
    X /= X(3);

    cout<<k<<" "<<K*PA*X<<endl;
    cout<<k<<" "<<K*PB*X<<endl;

    if( (PA*X)(2) > 0 && (PB*X)(2) > 0 ) return true;


    return false;

  };

//  for(int b = 0; b < 4; b++){

  docheck(check);
  for(size_t n = 0; n < N; n++){

    Vector3f XA, XB;
    XA << A[n].x, A[n].y, 1;
    XB << B[n].x, B[n].y, 1;

    Vector4f X = HDLT(K*PA, K*PB, XA, XB);
    X /= X(3);

    points.push_back(vec4(X(0), X(1), X(2), X(3)));

  }

  //}

  return points;

}

};  // End of Namespace mview
};  // End of Namespace tpose

#endif
