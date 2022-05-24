#include <TinyEngine/TinyEngine>
#include <TinyEngine/image>
#include <TinyEngine/color>

#include "../../source/triangulate.h"

using namespace glm;
using namespace std;

int main( int argc, char* args[] ) {

	if(argc < 3){
		cout<<"Specify two triangulation files"<<endl;
		exit(0);
	}

	Tiny::view.pointSize = 2.0f;
	Tiny::view.vsync = false;
	Tiny::view.antialias = 0;

	Tiny::window("Energy Based Image Triangulation, Nicholas Mcdonald 2022", 960, 540);

	tri::RATIO = 12.0/6.75;

	bool paused = true;
	bool showlines = false;

	Tiny::view.interface = [](){};
	Tiny::event.handler = [&](){

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_p)
			paused = !paused;

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_n)
			showlines = !showlines;

	};

	Texture tex(image::load("../../resource/imageB.png"));		//Load Texture with Image
	Square2D flat;																						//Create Primitive Model

	Shader image({"shader/image.vs", "shader/image.fs"}, {"in_Quad", "in_Tex"});
	Shader point({"shader/point.vs", "shader/point.fs"}, {"in_Position"});

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	tri::init();

	Buffer otherpointbuf;

	Triangle triangle;
	Instance triangleinstance(&triangle);

	TLineStrip tlinestrip;
	Instance linestripinstance(&tlinestrip);

	Shader triangleshader({"shader/triangle.vs", "shader/triangle.fs"}, {"in_Position"}, {"points", "index", "colacc", "otherpoints"});
	triangleshader.bind<vec2>("points", tri::pointbuf);
	triangleshader.bind<ivec4>("index", tri::trianglebuf);
	triangleshader.bind<vec2>("otherpoints", &otherpointbuf);

	Shader linestrip({"shader/linestrip.vs", "shader/linestrip.fs"}, {"in_Position"}, {"points", "index", "otherpoints"});
	linestrip.bind<vec2>("points", tri::pointbuf);
	linestrip.bind<ivec4>("index", tri::trianglebuf);
	linestrip.bind<vec2>("otherpoints", &otherpointbuf);

	Model pointmesh({"in_Position"});
	pointmesh.bind<vec2>("in_Position", tri::pointbuf);

	tri::triangulation trA, trB;
	trA.read(args[1], false);
	trB.read(args[2], false);

	tri::upload(&trB);
	otherpointbuf.fill(trA.points);
	pointmesh.SIZE = trA.NP;

	// Color Accumulation Buffers

	triangleshader.bind<ivec4>("colacc", tri::tcolaccbuf);

	float s = 0.0f;

	auto draw = [&](){

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);		//Load Texture
		triangleshader.uniform("mode", 2);
		triangleshader.uniform("K", trA.NT);
		triangleshader.uniform("s", s);
		triangleshader.uniform("RATIO", tri::RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, trA.NT);

	//	point.use();
	//	point.uniform("RATIO", tri::RATIO);
	//	pointmesh.render(GL_POINTS);

		if(showlines){
			linestrip.use();
			linestrip.uniform("RATIO", tri::RATIO);
			linestrip.uniform("s", s);
			linestripinstance.render(GL_LINE_STRIP, trA.NT);
		}

	};

	// Main Functions

	triangleshader.use();
	triangleshader.texture("imageTexture", tex);
	triangleshader.uniform("mode", 1);
	triangleshader.uniform("KTriangles", trA.NT);
	triangleshader.uniform("RATIO", tri::RATIO);
	triangleinstance.render(GL_TRIANGLE_STRIP, trA.NT);

	Tiny::view.pipeline = [&](){

		Tiny::view.target(color::white);				//Target Main Screen
		draw();

	};

	int NN = 0;
	float ds = 0.001f;
	Tiny::loop([&](){
		s += ds;
		if(s < 0 || s > 1) ds *= -1;
	});

	tri::quit();
	Tiny::quit();

	return 0;

}
