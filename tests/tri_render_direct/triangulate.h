// triangulate.h

using namespace glm;
using namespace std;

struct Triangulation : Model {

  vector<float> vertices;
  vector<double> coords;
  vector<GLuint> indices;
  Buffer pos, ind;

  Triangulation(const size_t K) : Model({"in_Position"}){

    std::vector<vec2> points;			//Coordinates for Delaunation

    points.emplace_back( -1.2/0.8f, -1 );
    points.emplace_back( -1.2/0.8f,  1 );
    points.emplace_back(  1.2/0.8f, -1 );
    points.emplace_back(  1.2/0.8f,  1 );

    while(points.size() < K/2)
      sample::disc(points, K, vec2(-12.0f/8.0f, -1.0f), vec2(12.0f/8.0f, 1.0f));

    for(size_t i = 0; i < points.size(); i++){
      vertices.push_back(points[i].x);
      vertices.push_back(points[i].y);
      coords.push_back(points[i].x);
      coords.push_back(points[i].y);
    }

    delaunator::Delaunator d(coords);			//Compute Delaunay Triangulation

    for(size_t i = 0; i < d.triangles.size()/3; i++){
      indices.push_back(d.triangles[3*i+0]);
      indices.push_back(d.triangles[3*i+1]);
      indices.push_back(d.triangles[3*i+1]);
      indices.push_back(d.triangles[3*i+2]);
      indices.push_back(d.triangles[3*i+2]);
      indices.push_back(d.triangles[3*i+0]);
  	}

    pos.fill(vertices);
    ind.fill(indices);

    bind<glm::vec2>("in_Position", &pos);
    index(&ind);
    SIZE = indices.size();

  }

};
