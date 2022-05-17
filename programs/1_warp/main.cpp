#include <TinyEngine/TinyEngine>
#include <TinyEngine/image>
#include <TinyEngine/color>

#include "../../source/triangulate.h"

using namespace std;
using namespace glm;

int main( int argc, char* args[] ) {

	Tiny::view.pointSize = 2.0f;
	Tiny::view.vsync = false;
	Tiny::view.antialias = 0;

	Tiny::window("Energy Based Image Triangulation, Nicholas Mcdonald 2022", 960, 540);
	tri::RATIO = 9.6/5.4;

	bool paused = true;
	bool donext = false;

	Tiny::event.handler = [&](){

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_p)
			paused = !paused;

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_n)
			donext = true;

	};
	Tiny::view.interface = [](){};

	Texture tex(image::load("../../resource/imageB.png"));		//Load Texture with Image
	Square2D flat;																						//Create Primitive Model

	vector<int> importlist = {
		1000,
		900,
		800,
		700,
		600,
		500,
		400,
		300,
		200,
		100,
		50
	};

	Shader image({"shader/image.vs", "shader/image.fs"}, {"in_Quad", "in_Tex"});
	Shader point({"shader/point.vs", "shader/point.fs"}, {"in_Position"});

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	// Shaders and Buffers

	tri::init();

	Shader triangleshader({"shader/triangle.vs", "shader/triangle.fs"}, {"in_Position"}, {"points", "index", "colacc", "colnum", "energy", "gradient", "nring"});
	triangleshader.bind<vec2>("points", tri::pointbuf);
	triangleshader.bind<ivec4>("index", tri::trianglebuf);
	triangleshader.bind<ivec4>("colacc", tri::tcolaccbuf);
	triangleshader.bind<int>("colnum", tri::tcolnumbuf);
	triangleshader.bind<int>("energy", tri::tenergybuf);
	triangleshader.bind<ivec2>("gradient", tri::pgradbuf);
	triangleshader.bind<int>("nring", tri::tnringbuf);

	Shader linestrip({"shader/linestrip.vs", "shader/linestrip.fs"}, {"in_Position"}, {"points", "index"});
	linestrip.bind<vec2>("points", tri::pointbuf);
	linestrip.bind<ivec4>("index", tri::trianglebuf);

	// SSBO Manipulation Compute Shaders (Reset / Average)

	Compute reset({"shader/reset.cs"}, {"colacc", "colnum", "energy", "gradient", "nring"});
	reset.bind<ivec4>("colacc", tri::tcolaccbuf);
	reset.bind<int>("colnum", tri::tcolnumbuf);
	reset.bind<int>("energy", tri::tenergybuf);
	reset.bind<ivec2>("gradient", tri::pgradbuf);
	reset.bind<int>("nring", tri::tnringbuf);

	Compute gradient({"shader/gradient.cs"}, {"index", "energy", "gradient"});
	gradient.bind<ivec4>("index", tri::trianglebuf);
	gradient.bind<int>("energy", tri::tenergybuf);
	gradient.bind<ivec2>("gradient", tri::pgradbuf);

	Compute shift({"shader/shift.cs"}, {"points", "gradient"});
	shift.bind<ivec4>("points", tri::pointbuf);
	shift.bind<ivec2>("gradient", tri::pgradbuf);

	// Triangulation and Models

	tri::triangulation tr(to_string(importlist.back())+".tri");
	tri::upload(&tr);
	importlist.pop_back();

	cout<<"Number of Triangles: "<<tr.NT<<endl;

	Triangle triangle;
	Instance triangleinstance(&triangle);

	TLineStrip tlinestrip;
	Instance linestripinstance(&tlinestrip);

	Model pointmesh({"in_Position"});
	pointmesh.bind<vec2>("in_Position", tri::pointbuf);

	// Convenience Lambdas

	auto doreset = [&](){

		reset.use();
		reset.uniform("NTriangles", 13*tr.NT);
		reset.uniform("NPoints", tr.NP);

		if((13*tr.NT) > tr.NP) reset.dispatch(1 + (13*tr.NT)/1024);
		else reset.dispatch(1 + tr.NP/1024);

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

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
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		shift.use();
		shift.uniform("NPoints", tr.NP);
		shift.uniform("RATIO", tri::RATIO);
		shift.dispatch(1 + tr.NP/1024);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	};

	auto draw = [&](){

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);		//Load Texture
		triangleshader.uniform("mode", 2);
		triangleshader.uniform("K", tr.NT);
		triangleshader.uniform("RATIO", tri::RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, tr.NT);

	//	point.use();
	//	point.uniform("RATIO", tri::RATIO);
	//	pointmesh.render(GL_POINTS, tr.NT);

		linestrip.use();
		linestrip.uniform("RATIO", tri::RATIO);
		linestripinstance.render(GL_LINE_STRIP, tr.NT);

	};

	// Main Functions

	Tiny::view.pipeline = [&](){

		Tiny::view.target(color::black);				//Target Main Screen

		doreset();
		draw();

	};

	Tiny::loop([&](){

		if(paused) return;

		// Compute Cost and Gradients, Shift Points

		doenergy();
		doshift();

		// Retrieve Data from Compute Shader

		tri::tenergybuf->retrieve((13*tr.NT), tri::err);
		tri::tcolnumbuf->retrieve((13*tr.NT), tri::cn);
		tri::pointbuf->retrieve(tr.points);

		if(donext){
	//	if( tri::geterr(&tr) < 1E-4 ){

			donext = false;
			cout<<"RETRIANGULATE"<<endl;

			paused = true;

			if(!importlist.empty()){

				tr.read(to_string(importlist.back())+".tri");
				tri::upload(&tr);
				importlist.pop_back();

				doreset();
				doenergy();

				draw();

			}

			else{

				Tiny::event.quit = true;
				tr.write("out.tri");
				return;

			}

		}

	});


	tri::quit();
	Tiny::quit();

	return 0;
}
