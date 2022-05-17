
// Point / Triangle Parameterization

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
