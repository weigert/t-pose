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

vector<vec2> points;			//Coordinates for Delaunation
vector<vec2> offset;
vector<ivec4> triangles;	//Triangle Point Indexing
vector<int> halfedges;	//Triangle Halfedge Indexing

void initialize( const int K = 1024 ){

  // Create a Delaunator!

	points.emplace_back(-1.2/0.8f,-1);
	points.emplace_back(-1.2/0.8f, 1);
	points.emplace_back( 1.2/0.8f,-1);
	points.emplace_back( 1.2/0.8f, 1);

	while(points.size() < K/2)
		sample::disc(points, K, vec2(-12.0f/8.0f, -1.0f), vec2(12.0f/8.0f, 1.0f));

	vector<double> coords;					//Coordinates for Delaunation
  for(size_t i = 0; i < points.size(); i++){
    coords.push_back(points[i].x);
    coords.push_back(points[i].y);
  }

	delaunator::Delaunator d(coords);			//Compute Delaunay Triangulation

	for(size_t i = 0; i < points.size(); i++){
		offset.push_back(points[i]);
	}

	pointbuf = new Buffer(offset);

  for(size_t i = 0; i < d.triangles.size()/3; i++){
		triangles.emplace_back(d.triangles[3*i+0], d.triangles[3*i+1], d.triangles[3*i+2], 0);
		trianglebuf = new Buffer(triangles);
		halfedges.push_back(d.halfedges[3*i+0]);
		halfedges.push_back(d.halfedges[3*i+1]);
		halfedges.push_back(d.halfedges[3*i+2]);
	}

}
