#include <TinyEngine/TinyEngine>
#include <TinyEngine/image>
#include <TinyEngine/color>

#include <iomanip>
#include <map>
#include <set>

#include "../../../source/triangulate.h"

using namespace std;
using namespace glm;

int main( int argc, char* args[] ) {

	Tiny::view.pointSize = 2.0f;
	Tiny::view.vsync = false;
	Tiny::view.antialias = 0;

	Tiny::window("Energy Based Image Triangulation, Nicholas Mcdonald 2022", 800, 450);
	tri::RATIO = 12.0/6.75;

	bool paused = true;
	bool viewlines = false;

	Tiny::event.handler = [&](){

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_p)
			paused = !paused;

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_n)
		viewlines = !viewlines;

	};
	Tiny::view.interface = [](){};

	Texture tex(image::load("../../../resource/imageA.png"));		//Load Texture with Image
	Square2D flat;																					//Create Primitive Model

	Shader image({"shader/image.vs", "shader/image.fs"}, {"in_Quad", "in_Tex"});
	Shader point({"shader/point.vs", "shader/point.fs"}, {"in_Position"});

	glDisable(GL_CULL_FACE);

	// Shaders and Buffers

	tri::init();

	Shader triangleshader({"shader/triangle.vs", "shader/triangle.fs"}, {"in_Position"}, {"points", "index", "colacc", "colnum", "tenergy", "penergy", "gradient", "nring"});
	triangleshader.bind<vec2>("points", tri::pointbuf);
	triangleshader.bind<ivec4>("index", tri::trianglebuf);
	triangleshader.bind<ivec4>("colacc", tri::tcolaccbuf);
	triangleshader.bind<int>("colnum", tri::tcolnumbuf);
	triangleshader.bind<int>("tenergy", tri::tenergybuf);
	triangleshader.bind<int>("penergy", tri::penergybuf);
	triangleshader.bind<ivec2>("gradient", tri::pgradbuf);
	triangleshader.bind<int>("nring", tri::tnringbuf);

	Shader linestrip({"shader/linestrip.vs", "shader/linestrip.fs"}, {"in_Position"}, {"points", "index"});
	linestrip.bind<vec2>("points", tri::pointbuf);
	linestrip.bind<ivec4>("index", tri::trianglebuf);

	// SSBO Manipulation Compute Shaders (Reset / Average)

	Compute reset({"shader/reset.cs"}, {"colacc", "colnum", "tenergy", "penergy" "gradient", "nring"});
	reset.bind<ivec4>("colacc", tri::tcolaccbuf);
	reset.bind<int>("colnum", tri::tcolnumbuf);
	reset.bind<int>("tenergy", tri::tenergybuf);
	reset.bind<int>("penergy", tri::penergybuf);
	reset.bind<int>("nring", tri::tnringbuf);

	Compute gradient({"shader/gradient.cs"}, {"index", "energy", "gradient"});
	gradient.bind<ivec4>("index", tri::trianglebuf);
	gradient.bind<int>("energy", tri::tenergybuf);
	gradient.bind<ivec2>("gradient", tri::pgradbuf);

	Compute shift({"shader/shift.cs"}, {"points", "gradient"});
	shift.bind<ivec4>("points", tri::pointbuf);
	shift.bind<ivec2>("gradient", tri::pgradbuf);

	// Triangulation and Models

	tri::triangulation tr;
	tri::upload(&tr, false);

	cout<<"Number of Triangles: "<<tr.NT<<endl;

	Triangle triangle;
	Instance triangleinstance(&triangle);

	TLineStrip tlinestrip;
	Instance linestripinstance(&tlinestrip);

	Model pointmesh({"in_Position"});
	pointmesh.bind<vec2>("in_Position", tri::pointbuf);

	// Convenience Lambdas

	auto computecolors = [&](){

		reset.use();
		reset.uniform("NTriangles", 13*tr.NT);
		reset.uniform("NPoints", tr.NP);
		reset.dispatch(1 + (13*tr.NT)/1024);

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);		//Load Texture
		triangleshader.uniform("mode", 0);
		triangleshader.uniform("KTriangles", tr.NT);
		triangleshader.uniform("RATIO", tri::RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, (13*tr.NT));

	};

	auto doenergy = [&](){

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);
		triangleshader.uniform("mode", 1);
		triangleshader.uniform("KTriangles", tr.NT);
		triangleshader.uniform("RATIO", tri::RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, (13*tr.NT));

	};

	auto doshift = [&](){

		gradient.use();
		gradient.uniform("KTriangles", tr.NT);
		gradient.uniform("RATIO", tri::RATIO);
		gradient.dispatch(1 + tr.NT/1024);

		shift.use();
		shift.uniform("NPoints", tr.NP);
		shift.uniform("RATIO", tri::RATIO);
		shift.dispatch(1 + tr.NP/1024);

	};

	auto draw = [&](){

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);		//Load Texture
		triangleshader.uniform("mode", 2);
		triangleshader.uniform("K", tr.NT);
		triangleshader.uniform("RATIO", tri::RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, tr.NT);

	//	point.use();
	//	point.uniform("RATIO", RATIO);
	//	pointmesh.render(GL_POINTS, tr.NT);

		if(viewlines){
			linestrip.use();
			linestrip.uniform("RATIO", tri::RATIO);
			linestripinstance.render(GL_LINE_STRIP, tr.NT);
		}

	};

	computecolors();

	// Main Functions

	Tiny::view.pipeline = [&](){

		Tiny::view.target(color::black);				//Target Main Screen

		computecolors();
		draw();

	};

	Tiny::loop([&](){

		if(paused) return;

		// Compute Cost and Gradients, Shift Points

		doenergy();
		doshift();

		// Retrieve Data from Compute Shader

		tri::tenergybuf->retrieve((13*tr.NT), tri::terr);
		tri::penergybuf->retrieve((13*tr.NT), tri::perr);
		tri::tcolnumbuf->retrieve((13*tr.NT), tri::cn);
		tri::pointbuf->retrieve(tr.points);

		// TOPOLOGICAL OPTIMIZATIONS

		bool updated = false;

		#include "strat/flip_set_maxenergy.h"
		//#include "strat/flip_delaunay.h"

		if(updated){
			if(tr.NT >= 1000) paused = true;
			cout<<tr.NT<<" "<<std::setprecision(16)<<gettoterr( &tr )<<endl;
			tri::upload(&tr, false);
		}

	});

	tri::quit();
	Tiny::quit();

	return 0;
}
