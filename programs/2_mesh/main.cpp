#include <TinyEngine/TinyEngine>
#include <TinyEngine/image>
#include <TinyEngine/color>
#include <TinyEngine/camera>

#include "../../source/triangulate.h"
#include "../../source/unproject.h"

using namespace glm;
using namespace std;
using namespace Eigen;

int main( int argc, char* args[] ) {

	if(argc < 3){
		cout<<"Specify two triangulation files"<<endl;
		exit(0);
	}

	// Setup Window

	Tiny::view.pointSize = 2.0f;

	Tiny::window("Energy Based Image Triangulation, Nicholas Mcdonald 2022", 960, 540);

	tri::RATIO = 12.0/6.75;

	// Setup Camera

	cam::far = 100.0f;                          //Projection Matrix Far-Clip
	cam::near = 0.001f;
	cam::moverate = 0.05f;
	cam::FOV = 0.5;
	cam::init();
	cam::look = vec3(0,0,0);
	cam::update();

	// Setup 3D Point Buf!!!

	vector<vec4> point3D;
	Buffer point3Dbuf;

	bool paused = true;
	bool showlines = false;

	Tiny::view.interface = [](){};
	Tiny::event.handler = [&](){

		cam::handler();

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_p)
			paused = !paused;

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_n)
			showlines = !showlines;

	};

	Texture tex(image::load("../../resource/imageA.png"));		//Load Texture with Image
	Square2D flat;																						//Create Primitive Model

	Shader image({"shader/image.vs", "shader/image.fs"}, {"in_Quad", "in_Tex"});
	Shader point({"shader/point.vs", "shader/point.fs"}, {"in_Position"});

	glDisable(GL_CULL_FACE);
	//glDisable(GL_DEPTH_TEST);

	tri::init();

	Triangle triangle;
	Instance triangleinstance(&triangle);

	TLineStrip tlinestrip;
	Instance linestripinstance(&tlinestrip);

	Shader triangleshader({"shader/triangle.vs", "shader/triangle.fs"}, {"in_Position"}, {"points", "index", "colacc", "otherpoints"});
	triangleshader.bind<vec4>("points", &point3Dbuf);
	triangleshader.bind<ivec4>("index", tri::trianglebuf);

	Shader linestrip({"shader/linestrip.vs", "shader/linestrip.fs"}, {"in_Position"}, {"points", "index", "otherpoints"});
	linestrip.bind<vec4>("points", &point3Dbuf);
	linestrip.bind<ivec4>("index", tri::trianglebuf);

	Model pointmesh({"in_Position"});
	pointmesh.bind<vec4>("in_Position", &point3Dbuf);

	tri::triangulation trA, trB;
	trA.read(args[1], false);
	trB.read(args[2], false);

	tri::upload(&trB);
	pointmesh.SIZE = trA.NP;

	// Load the Enery, Erase Bad Triangles




	vector<float> TEN;
	vector<float> PEN;
	vector<int> CN;

	ifstream in("energy1500.txt", ios::in);
	if(in.is_open()){

		// Load the Energy, then delete bad triangles and points.

		cout<<"LOADING ENERGY"<<endl;

		string pstring;
		int state = 0;

	  vec2 pA = vec2(0);
	  vec2 pB = vec2(0);

		while(!in.eof()){

	    getline(in, pstring);
	    if(in.eof())
	      break;

			if(pstring == "TENERGY"){
				state = 0;
				continue;
			}

			if(pstring == "PENERGY"){
				state = 1;
				continue;
			}

			if(pstring == "CN"){
				state = 2;
				continue;
			}

			if(state == 0){
				float _TEN;
				int m = sscanf(pstring.c_str(), "%f", &_TEN);
				if(m == 1) TEN.push_back(_TEN);
			}

			if(state == 1){
				float _PEN;
				int m = sscanf(pstring.c_str(), "%f", &_PEN);
				if(m == 1) PEN.push_back(_PEN);
			}

			if(state == 2){
				float _CN;
				int m = sscanf(pstring.c_str(), "%f", &_CN);
				if(m == 1) CN.push_back(_CN);
			}

	  }

		in.close();

	}

	vector<vec2> tempA = trA.points;

	for(auto& pA: tempA){
		pA.x = 0.5f*(pA.x / (12.0/6.75) + 1.0f)*1200;
		pA.y = 0.5f*(-pA.y + 1.0f)*675;
		pA /= vec2(1200);
	}

	vector<vec2> tempB = trB.points;

	for(auto& pB: tempB){
		pB.x = 0.5f*(pB.x / (12.0/6.75) + 1.0f)*1200;
		pB.y = 0.5f*(-pB.y + 1.0f)*675;
		pB /= vec2(1200);
	}

	// Prune Bad Triangles

	for(size_t i = 0; i < trA.triangles.size(); i++){

		float _TEN = sqrt(float(TEN[i])/(float)CN[i])/255.0f;
    float _PEN = 0;
    _PEN += PEN[trA.triangles[i].x];
    _PEN += PEN[trA.triangles[i].y];
    _PEN += PEN[trA.triangles[i].z];

	//	cout<<_TEN<<" "<<_PEN<<endl;

    if(abs(_TEN) < abs(_PEN/1000000)
		|| (
			(tempA[trA.triangles[i].x].x <= -tri::RATIO) ||
	    (tempA[trA.triangles[i].x].x >= tri::RATIO) ||
	    (tempA[trA.triangles[i].x].y <= -1) ||
	    (tempA[trA.triangles[i].x].y >= 1) ||
	    (tempB[trA.triangles[i].x].x <= -tri::RATIO) ||
	    (tempB[trA.triangles[i].x].x >= tri::RATIO) ||
	    (tempB[trA.triangles[i].x].y <= -1) ||
	    (tempB[trA.triangles[i].x].y >= 1) ||
			(tempA[trA.triangles[i].y].x <= -tri::RATIO) ||
			(tempA[trA.triangles[i].y].x >= tri::RATIO) ||
			(tempA[trA.triangles[i].y].y <= -1) ||
			(tempA[trA.triangles[i].y].y >= 1) ||
			(tempB[trA.triangles[i].y].x <= -tri::RATIO) ||
			(tempB[trA.triangles[i].y].x >= tri::RATIO) ||
			(tempB[trA.triangles[i].y].y <= -1) ||
			(tempB[trA.triangles[i].y].y >= 1) ||
			(tempA[trA.triangles[i].z].x <= -tri::RATIO) ||
			(tempA[trA.triangles[i].z].x >= tri::RATIO) ||
			(tempA[trA.triangles[i].z].y <= -1) ||
			(tempA[trA.triangles[i].z].y >= 1) ||
			(tempB[trA.triangles[i].z].x <= -tri::RATIO) ||
			(tempB[trA.triangles[i].z].x >= tri::RATIO) ||
			(tempB[trA.triangles[i].z].y <= -1) ||
			(tempB[trA.triangles[i].z].y >= 1)
		)){
			trA.triangles.erase(trA.triangles.begin()+i);
			trA.colors.erase(trA.colors.begin()+i);
			trB.triangles.erase(trB.triangles.begin()+i);
			trB.colors.erase(trB.colors.begin()+i);
			TEN.erase(TEN.begin()+i);
			CN.erase(CN.begin()+i);

			trA.NT--;
			trB.NT--;
		//	cout<<"ERASED "<<i<<endl;
			//prune
			i--;
		}

	}

	cout<<"Pruning Points"<<endl;

	for(size_t p = 0; p < trA.points.size(); p++){

		// Iterate over triangles
		bool found = false;

		for(size_t t = 0; t < trA.triangles.size(); t++){
			if(trA.triangles[t].x == p) found = true;
			if(trA.triangles[t].y == p) found = true;
			if(trA.triangles[t].z == p) found = true;
			if(found) break;
		}

		if(!found){ // Prune

			for(size_t t = 0; t < trA.triangles.size(); t++){
				if(trA.triangles[t].x >= p) trA.triangles[t].x--;
				if(trA.triangles[t].y >= p) trA.triangles[t].y--;
				if(trA.triangles[t].z >= p) trA.triangles[t].z--;
				if(trB.triangles[t].x >= p) trB.triangles[t].x--;
				if(trB.triangles[t].y >= p) trB.triangles[t].y--;
				if(trB.triangles[t].z >= p) trB.triangles[t].z--;
			}

			trA.points.erase(trA.points.begin() + p);
			trB.points.erase(trB.points.begin() + p);
			p--;

			trA.NP--;
			trB.NP--;

		}

	}

	tri::upload(&trA);




//	tri::trianglebuf->fill(trA.triangles);



	// Fill the Point3D Buf


	// Normalize these Data-Points

	tempA = trA.points;

	for(auto& pA: tempA){
		pA.x = 0.5f*(pA.x / (12.0/6.75) + 1.0f)*1200;
		pA.y = 0.5f*(-pA.y + 1.0f)*675;
		pA /= vec2(1200);
	}

	tempB = trB.points;

	for(auto& pB: tempB){
		pB.x = 0.5f*(pB.x / (12.0/6.75) + 1.0f)*1200;
		pB.y = 0.5f*(-pB.y + 1.0f)*675;
		pB /= vec2(1200);
	}

	Matrix3f K = unp::Camera();
	Matrix3f F = unp::F_RANSAC(tempA, tempB);


//Matrix3f F;
//F << -0.936791,      2.24,  -26.1548,
//1.03284,   1.77101,  -13.5179,
//26.3407,   10.5485,         1;

	point3D = unp::triangulate(F, K, tempA, tempB);
	point3Dbuf.fill(point3D);

	// Color Accumulation Buffers

	triangleshader.bind<ivec4>("colacc", tri::tcolaccbuf);

	auto draw = [&](){

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);		//Load Texture
		triangleshader.uniform("K", trA.NT);
		triangleshader.uniform("RATIO", tri::RATIO);
		triangleshader.uniform("vp", cam::vp);
		triangleshader.uniform("model", rotate(mat4(1.0f), 3.14159265f, vec3(0,0,1)));
		triangleinstance.render(GL_TRIANGLE_STRIP, trA.NT);

		point.use();
		point.uniform("RATIO", tri::RATIO);
		point.uniform("vp", cam::vp);
		point.uniform("model", rotate(mat4(1.0f), 3.14159265f, vec3(0,0,1)));
		pointmesh.render(GL_POINTS);

		if(showlines){
			linestrip.use();
			linestrip.uniform("RATIO", tri::RATIO);
			linestrip.uniform("vp", cam::vp);
			linestrip.uniform("model", rotate(mat4(1.0f), 3.14159265f, vec3(0,0,1)));
			linestripinstance.render(GL_LINE_STRIP, trA.NT);
		}

	};

	// Main Functions

	triangleshader.use();
	triangleshader.texture("imageTexture", tex);
	triangleshader.uniform("KTriangles", trA.NT);
	triangleshader.uniform("RATIO", tri::RATIO);
	triangleshader.uniform("vp", cam::vp);
	triangleinstance.render(GL_TRIANGLE_STRIP, trA.NT);

	Tiny::view.pipeline = [&](){

		Tiny::view.target(color::black);				//Target Main Screen
		draw();

	};

	Tiny::loop([&](){});

	tri::quit();
	Tiny::quit();

	return 0;

}
