#ifndef TPOSE_UTILITY
#define TPOSE_UTILITY

// utility.h

#include <Eigen/Dense>
#include <Eigen/Eigenvalues>

/*
================================================================================
											Barycentric Coordinate Transform
================================================================================
*/

// Compute the Parameters of a Point in a Triangle

glm::vec3 barycentric(glm::vec2 p, glm::ivec4 t, std::vector<glm::vec2>& v){

	glm::mat3 R(
		1, v[t.x].x, v[t.x].y,
		1, v[t.y].x, v[t.y].y,
		1, v[t.z].x, v[t.z].y
	);

	if(abs(determinant(R)) < 1E-8) return glm::vec3(1,1,1);
	return inverse(R)*glm::vec3(1, p.x, p.y);

}

// Check if a Point is in a Triangle

bool intriangle( glm::vec2 p, glm::ivec4 t, std::vector<glm::vec2>& v){

	if(length(v[t.x] - v[t.y]) == 0) return false;
	if(length(v[t.y] - v[t.z]) == 0) return false;
	if(length(v[t.z] - v[t.x]) == 0) return false;

	glm::vec3 s = barycentric(p, t, v);
	if(s.x < 0 || s.x > 1) return false;
	if(s.y < 0 || s.y > 1) return false;
	if(s.z < 0 || s.z > 1) return false;
	return true;

}

// Map Point in Triangle

glm::vec2 cartesian(glm::vec3 s, glm::ivec4 t, std::vector<glm::vec2>& v){

	return s.x * v[t.x] + s.y * v[t.y] + s.z * v[t.z];

}

/*
================================================================================
									Polynomial Evaluation / Root-Finding
================================================================================
*/

// Horner's Method Polynomial Evaluation

double horner(double x, std::vector<double> a){

  double result = a[a.size()-1];
  for(int i = a.size()-2; i >= 0; i--)
    result = result*x + a[i];
  return result;

}

// Companion Matrix Root Computation (with complex)

Eigen::VectorXcd roots(std::vector<double> a){

  const size_t K = a.size()-1;
  Eigen::MatrixXd C = Eigen::MatrixXd::Zero(K, K);

  for(size_t k = 0; k < K; k++)
    C(k,K-1) = -a[k]/a[K];

  for(size_t k = 1; k < K; k++)
    C(k,k-1) = 1;

  Eigen::EigenSolver<Eigen::MatrixXd> CS(C);
  return CS.eigenvalues();

}

// Real Roots Only! (Iterable)

std::vector<double> realroots(std::vector<double> a){

	Eigen::VectorXcd R = roots(a);	// get Complex Roots

  std::vector<double> rR;
  for(size_t r = 0; r < a.size()-1; r++)
    if(R(r).imag() == 0) rR.push_back(R(r).real());

	// Estimate Refinement with Newton's Method

  for(auto& r: rR)
  for(size_t n = 0; n < 25; n++)
    r -= horner(r, a)/horner(r, {a[1]*1.0, a[2]*2.0, a[3]*3.0, a[4]*4.0, a[5]*5.0, a[6]*6.0});

  return rR;

}

/*
================================================================================
											Eigen / GLM Matrix Conversions
================================================================================
*/

glm::mat3 fromEigen3(Eigen::Matrix3f M){
  glm::mat3 T;
  T = { M(0,0), M(0,1), M(0,2),
        M(1,0), M(1,1), M(1,2),
        M(2,0), M(2,1), M(2,2) };
  //Tranpose because of Column Major Ordering
  return glm::transpose(T);
}

Eigen::Matrix3f toEigen(glm::mat3 M){
  Eigen::Matrix3f T;
  T <<  M[0][0], M[0][1], M[0][2],
        M[1][0], M[1][1], M[1][2],
        M[2][0], M[2][1], M[2][2];
  //Tranpose because of Row Major Ordering
  return T.transpose();
}

glm::mat4 fromEigen(Eigen::Matrix4f M){
  glm::mat4 T;
  T = { M(0,0), M(0,1), M(0,2), M(0,3),
        M(1,0), M(1,1), M(1,2), M(1,3),
        M(2,0), M(2,1), M(2,2), M(2,3),
        M(3,0), M(3,1), M(3,2), M(3,3) };
  //Tranpose because of Column Major Ordering
  return glm::transpose(T);
}

Eigen::Matrix4f toEigen(glm::mat4 M){
  Eigen::Matrix4f T;
  T <<  M[0][0], M[0][1], M[0][2], M[0][3],
        M[1][0], M[1][1], M[1][2], M[1][3],
        M[2][0], M[2][1], M[2][2], M[2][3],
        M[3][0], M[3][1], M[3][2], M[3][3];
  //Tranpose because of Row Major Ordering
  return T.transpose();
}

#endif
