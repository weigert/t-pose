#include <TinyEngine/TinyEngine>
#include <TinyEngine/image>
#include <TinyEngine/color>
#include <TinyEngine/parse>

#include <iomanip>
#include <map>
#include <set>

#include <tpose/tpose>
#include <tpose/triangulation>

using namespace std;
using namespace glm;

int main( int argc, char* args[] ) {

	parse::get(argc, args);

	SDL_Surface* IMG = NULL;
	if(!parse::option.contains("i")){
		cout<<"Please specify an input image with -i."<<endl;
		exit(0);
	}
	else IMG = IMG_Load(parse::option["i"].c_str());
	if(IMG == NULL){
		cout<<"Failed to load image."<<endl;
		exit(0);
	}

	Tiny::view.pointSize = 2.0f;
	Tiny::view.vsync = false;
	Tiny::view.antialias = 0;

	Tiny::window("Energy Based Image Triangulation, Nicholas Mcdonald 2022", IMG->w/1.5f, IMG->h/1.5f);
	tpose::RATIO = (float)IMG->w/(float)IMG->h;

	bool paused = true;
	bool viewlines = false;

	Tiny::event.handler = [&](){

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_p)
			paused = !paused;

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_n)
		viewlines = !viewlines;

	};
	Tiny::view.interface = [](){};

	Texture tex(IMG);		//Load Texture with Image
	Square2D flat;																					//Create Primitive Model
	Shader image({"shader/image.vs", "shader/image.fs"}, {"in_Quad", "in_Tex"});

	glDisable(GL_CULL_FACE);

	// Shaders and Buffers

	tpose::init();

	Shader triangleshader({"shader/triangle.vs", "shader/triangle.fs"}, {"in_Position"}, {"points", "index", "colacc", "colnum", "tenergy", "penergy", "gradient", "nring"});
	triangleshader.bind<vec2>("points", tpose::pointbuf);
	triangleshader.bind<ivec4>("index", tpose::trianglebuf);
	triangleshader.bind<ivec4>("colacc", tpose::tcolaccbuf);
	triangleshader.bind<int>("colnum", tpose::tcolnumbuf);
	triangleshader.bind<int>("tenergy", tpose::tenergybuf);
	triangleshader.bind<int>("penergy", tpose::penergybuf);
	triangleshader.bind<ivec2>("gradient", tpose::pgradbuf);
	triangleshader.bind<int>("nring", tpose::tnringbuf);

	Shader linestrip({"shader/linestrip.vs", "shader/linestrip.fs"}, {"in_Position"}, {"points", "index"});
	linestrip.bind<vec2>("points", tpose::pointbuf);
	linestrip.bind<ivec4>("index", tpose::trianglebuf);

	Compute gradient({"shader/gradient.cs"}, {"index", "energy", "gradient"});
	gradient.bind<ivec4>("index", tpose::trianglebuf);
	gradient.bind<int>("energy", tpose::tenergybuf);
	gradient.bind<ivec2>("gradient", tpose::pgradbuf);

	Compute shift({"shader/shift.cs"}, {"points", "gradient"});
	shift.bind<ivec4>("points", tpose::pointbuf);
	shift.bind<ivec2>("gradient", tpose::pgradbuf);

	// Triangulation and Models

	tpose::triangulation tr;
	tpose::upload(&tr, false);

	cout<<"Number of Triangles: "<<tr.NT<<endl;

	Triangle triangle;
	Instance triangleinstance(&triangle);

	TLineStrip tlinestrip;
	Instance linestripinstance(&tlinestrip);

	Model pointmesh({"in_Position"});
	pointmesh.bind<vec2>("in_Position", tpose::pointbuf);

	// Convenience Lambdas

	auto computecolors = [&](){

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);		//Load Texture
		triangleshader.uniform("mode", 0);
		triangleshader.uniform("KTriangles", tr.NT);
		triangleshader.uniform("RATIO", tpose::RATIO);
		triangleinstance.render(GL_TRIANGLES, (13*tr.NT));

	};

	auto doenergy = [&](){

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);
		triangleshader.uniform("mode", 1);
		triangleshader.uniform("KTriangles", tr.NT);
		triangleshader.uniform("RATIO", tpose::RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, (13*tr.NT));

	};

	auto doshift = [&](){

		gradient.use();
		gradient.uniform("KTriangles", tr.NT);
		gradient.uniform("RATIO", tpose::RATIO);
		gradient.dispatch(1 + tr.NT/1024);

		shift.use();
		shift.uniform("NPoints", tr.NP);
		shift.uniform("RATIO", tpose::RATIO);
		shift.dispatch(1 + tr.NP/1024);

	};

	auto draw = [&](){

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);		//Load Texture
		triangleshader.uniform("mode", 2);
		triangleshader.uniform("KTriangles", tr.NT);
		triangleshader.uniform("RATIO", tpose::RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, tr.NT);

		if(viewlines){
			linestrip.use();
			linestrip.uniform("RATIO", tpose::RATIO);
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

		tpose::tenergybuf->retrieve((13*tr.NT), tpose::terr);
//		tpose::penergybuf->retrieve((13*tr.NT), tpose::perr);
//		tpose::tcolnumbuf->retrieve((13*tr.NT), tpose::cn);
		tpose::pointbuf->retrieve(tr.points);

		// TOPOLOGICAL OPTIMIZATIONS

		bool updated = false;

		#include "strat/flip_set_maxenergy.h"
		//#include "strat/flip_delaunay.h"

		if(updated){
			if(tr.NT >= 1000) paused = true;
			cout<<tr.NT<<" "<<std::setprecision(16)<<gettoterr( &tr )<<endl;
			tpose::upload(&tr, false);
		}

	});

	tpose::quit();
	Tiny::quit();

	return 0;
}
