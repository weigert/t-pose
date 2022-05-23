// triangulate.h

using namespace glm;
using namespace std;

struct Triangle : Model {
	Buffer vert;
	Triangle():Model({"vert"}),
	vert({1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f}){
		bind<vec3>("vert", &vert);
		SIZE = 3;
	}
};

Buffer* trianglebuf;
Buffer* pointbuf;

void initialize( const int K = 2048 ){

  // Create a Delaunator!

	vector<vec2> points;			//Coordinates for Delaunation
	while(points.size() < K/2)
		sample::disc(points, K, vec2(-12.0f/8.0f, -1.0f), vec2(12.0f/8.0f, 1.0f));

	points.emplace_back(-1.2/0.8f,-1);
	points.emplace_back(-1.2/0.8f, 1);
	points.emplace_back( 1.2/0.8f,-1);
	points.emplace_back( 1.2/0.8f, 1);

	pointbuf = new Buffer(points);

	vector<double> coords;					//Coordinates for Delaunation
  for(size_t i = 0; i < points.size(); i++){
    coords.push_back(points[i].x);
    coords.push_back(points[i].y);
  }

	delaunator::Delaunator d(coords);			//Compute Delaunay Triangulation

	vector<ivec3> triangles;	//Triangle Point Indexing
  for(size_t i = 0; i < d.triangles.size()/3; i++)
		triangles.emplace_back(d.triangles[3*i+2], d.triangles[3*i+1], d.triangles[3*i+0]);
	trianglebuf = new Buffer(triangles);

}
