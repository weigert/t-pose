mat3 fromEigen3(Matrix3f M){
  mat3 T;
  T = { M(0,0), M(0,1), M(0,2),
        M(1,0), M(1,1), M(1,2),
        M(2,0), M(2,1), M(2,2) };
  //Tranpose because of Column Major Ordering
  return transpose(T);
}

Matrix3f toEigen(mat3 M){
  Matrix3f T;
  T <<  M[0][0], M[0][1], M[0][2],
        M[1][0], M[1][1], M[1][2],
        M[2][0], M[2][1], M[2][2];
  //Tranpose because of Row Major Ordering
  return T.transpose();
}

mat4 fromEigen(Matrix4f M){
  mat4 T;
  T = { M(0,0), M(0,1), M(0,2), M(0,3),
        M(1,0), M(1,1), M(1,2), M(1,3),
        M(2,0), M(2,1), M(2,2), M(2,3),
        M(3,0), M(3,1), M(3,2), M(3,3) };
  //Tranpose because of Column Major Ordering
  return transpose(T);
}

Matrix4f toEigen(mat4 M){
  Matrix4f T;
  T <<  M[0][0], M[0][1], M[0][2], M[0][3],
        M[1][0], M[1][1], M[1][2], M[1][3],
        M[2][0], M[2][1], M[2][2], M[2][3],
        M[3][0], M[3][1], M[3][2], M[3][3];
  //Tranpose because of Row Major Ordering
  return T.transpose();
}
