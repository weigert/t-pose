#include <TinyEngine/TinyEngine>
#include <TinyEngine/image>
#include <TinyEngine/color>

//const float RATIO = 6.0f/9.0f;
float RATIO = 12.0f/6.75f;

#include "triangulate.h"

int main( int argc, char* args[] ) {

	Tiny::view.pointSize = 2.0f;
	Tiny::view.vsync = false;
	Tiny::view.antialias = 0;
//	Tiny::benchmark = true;

	Tiny::window("Energy Based Image Triangulation, Nicholas Mcdonald 2022", 960, 540);

	bool paused = true;
	bool showlines = false;

	vector<vec2> otherpoints;

	Tiny::event.handler = [&](){

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_p)
			paused = !paused;

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_n){
			showlines = !showlines;

		}

	};
	Tiny::view.interface = [](){};

	Texture tex(image::load("../../resource/imageA.png"));		//Load Texture with Image
	Square2D flat;																						//Create Primitive Model

	Shader image({"shader/image.vs", "shader/image.fs"}, {"in_Quad", "in_Tex"});
	Shader point({"shader/point.vs", "shader/point.fs"}, {"in_Position"});

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	initbufs();
	read("2000.tri");
	otherpoints = points;
	read("out.tri");

	Buffer otherpointbuf(otherpoints);

	Triangle triangle;
	Instance triangleinstance(&triangle);
	triangleinstance.SIZE = KTriangles;

	TLineStrip tlinestrip;
	Instance linestripinstance(&tlinestrip);
	linestripinstance.SIZE = KTriangles;

	Shader triangleshader({"shader/triangle.vs", "shader/triangle.fs"}, {"in_Position"}, {"points", "index", "colacc", "otherpoints"});
	triangleshader.bind<vec2>("points", pointbuf);
	triangleshader.bind<ivec4>("index", trianglebuf);
	triangleshader.bind<vec2>("otherpoints", &otherpointbuf);

	Shader linestrip({"shader/linestrip.vs", "shader/linestrip.fs"}, {"in_Position"}, {"points", "index", "otherpoints"});
	linestrip.bind<vec2>("points", pointbuf);
	linestrip.bind<ivec4>("index", trianglebuf);
	linestrip.bind<vec2>("otherpoints", &otherpointbuf);

	Model pointmesh({"in_Position"});
	pointmesh.bind<vec2>("in_Position", pointbuf);
	pointmesh.SIZE = NPoints;

	// Color Accumulation Buffers

	triangleshader.bind<ivec4>("colacc", tcolaccbuf);

	float s = 0.0f;

	auto draw = [&](){

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);		//Load Texture
		triangleshader.uniform("mode", 2);
		triangleshader.uniform("K", KTriangles);
		triangleshader.uniform("s", s);
		triangleshader.uniform("RATIO", RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, KTriangles);

	//	point.use();
	//	point.uniform("RATIO", RATIO);
	//	pointmesh.render(GL_POINTS);

		if(showlines){
			linestrip.use();
			linestrip.uniform("RATIO", RATIO);
			linestrip.uniform("s", s);
			linestripinstance.render(GL_LINE_STRIP, KTriangles);
		}

	};

	// Main Functions

	triangleshader.use();
	triangleshader.texture("imageTexture", tex);
	triangleshader.uniform("mode", 1);
	triangleshader.uniform("KTriangles", KTriangles);
	triangleshader.uniform("RATIO", RATIO);
	triangleinstance.render(GL_TRIANGLE_STRIP, NTriangles);

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

	delete[] err;
	delete[] cn;

	delete trianglebuf;
	delete pointbuf;
	delete tcolaccbuf;
	delete tcolnumbuf;
	delete tenergybuf;
	delete pgradbuf;

	Tiny::quit();

	return 0;
}
