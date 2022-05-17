#include <TinyEngine/TinyEngine>
#include <TinyEngine/image>
#include <TinyEngine/color>
#include <TinyEngine/camera>

using namespace std;
using namespace glm;

int main( int argc, char* args[] ) {

	if(argc < 2){
		cout<<"Please specify input file"<<endl;
		exit(0);
	}

	Tiny::view.pointSize = 2.0f;
	Tiny::view.vsync = false;
	Tiny::view.antialias = 0;

	Tiny::window("Pointcloud Viewer, Nicholas Mcdonald 2022", 960, 540);

	cam::far = 100.0f;                          //Projection Matrix Far-Clip
	cam::near = 0.1f;
	cam::moverate = 0.05f;
	cam::init();

	Tiny::event.handler = cam::handler;
	Tiny::view.interface = [](){};

	Shader particle({"shader/particle.vs", "shader/particle.fs"}, {"in_Position"});

	// Load Points

	vector<vec3> points;
	ifstream in(args[1], ios::in);
	if(!in.is_open()){
		cout<<"Failed to open file "<<args[1]<<endl;
		exit(0);
	}

	vec3 p = vec3(0);
	string pstring;

	while(!in.eof()){

		getline(in, pstring);
		if(in.eof())
			break;

		int m = sscanf(pstring.c_str(), "%f %f %f", &p.x, &p.y, &p.z);
		if(m == 3) points.push_back(p);

	}

	in.close();

	cout << "Loaded " << points.size() << " points" << endl;

	






	// Loaded Points

	Buffer pointbuf(points);
	Model pointmesh({"in_Position"});
	pointmesh.bind<vec3>("in_Position", &pointbuf);
	pointmesh.SIZE = points.size();

	Tiny::view.pipeline = [&](){

		Tiny::view.target(color::black);				//Target Main Screen

		particle.use();
		particle.uniform("vp", cam::vp);
		pointmesh.render(GL_POINTS);

	};

	Tiny::loop([&](){});
	Tiny::quit();

	return 0;
}
