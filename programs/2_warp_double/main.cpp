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

	initbufs();
	read(to_string(importlist.back())+".tri");
	importlist.pop_back();

	Triangle triangle;
	Instance triangleinstance(&triangle);
	triangleinstance.SIZE = KTriangles;

	TLineStrip tlinestrip;
	Instance linestripinstance(&tlinestrip);
	linestripinstance.SIZE = KTriangles;

	Shader triangleshader({"shader/triangle.vs", "shader/triangle.fs"}, {"in_Position"}, {"points", "index", "colacc", "colnum", "energy"});
	triangleshader.bind<vec2>("points", pointbuf);
	triangleshader.bind<ivec4>("index", trianglebuf);

	Shader linestrip({"shader/linestrip.vs", "shader/linestrip.fs"}, {"in_Position"}, {"points", "index"});
	linestrip.bind<vec2>("points", pointbuf);
	linestrip.bind<ivec4>("index", trianglebuf);

	Model pointmesh({"in_Position"});
	pointmesh.bind<vec2>("in_Position", pointbuf);
	pointmesh.SIZE = NPoints;

	// Color Accumulation Buffers

	triangleshader.bind<ivec4>("colacc", tcolaccbuf);
	triangleshader.bind<int>("colnum", tcolnumbuf);
	triangleshader.bind<int>("energy", tenergybuf);

	// SSBO Manipulation Compute Shaders (Reset / Average)

	Compute reset({"shader/reset.cs"}, {"colacc", "colnum", "energy", "gradient"});
	reset.bind<ivec4>("colacc", tcolaccbuf);
	reset.bind<int>("colnum", tcolnumbuf);
	reset.bind<int>("energy", tenergybuf);
	reset.bind<ivec2>("gradient", pgradbuf);

	Compute gradient({"shader/gradient.cs"}, {"index", "energy", "gradient"});
	gradient.bind<ivec4>("index", trianglebuf);
	gradient.bind<int>("energy", tenergybuf);
	gradient.bind<ivec2>("gradient", pgradbuf);

	Compute shift({"shader/shift.cs"}, {"points", "gradient"});
	shift.bind<ivec4>("points", pointbuf);
	shift.bind<ivec2>("gradient", pgradbuf);

	// Convenience Lambdas

	auto doreset = [&](){

		reset.use();
		reset.uniform("NTriangles", NTriangles);
		reset.uniform("NPoints", NPoints);

		if(NTriangles > NPoints) reset.dispatch(1 + NTriangles/1024);
		else reset.dispatch(1 + NPoints/1024);

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);		//Load Texture
		triangleshader.uniform("mode", 0);
		triangleshader.uniform("KTriangles", KTriangles);
		triangleshader.uniform("RATIO", RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, NTriangles);

	};

	auto doenergy = [&](){

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);
		triangleshader.uniform("mode", 1);
		triangleshader.uniform("KTriangles", KTriangles);
		triangleshader.uniform("RATIO", RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, NTriangles);

	};

	auto doshift = [&](){

		gradient.use();
		gradient.uniform("KTriangles", KTriangles);
		gradient.uniform("RATIO", RATIO);
		gradient.dispatch(1 + KTriangles/1024);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		shift.use();
		shift.uniform("NPoints", NPoints);
		shift.uniform("RATIO", RATIO);
		shift.dispatch(1 + NPoints/1024);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	};

	auto draw = [&](){

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);		//Load Texture
		triangleshader.uniform("mode", 2);
		triangleshader.uniform("K", KTriangles);
		triangleshader.uniform("RATIO", RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, KTriangles);

	//	point.use();
	//	point.uniform("RATIO", RATIO);
	//	pointmesh.render(GL_POINTS);

		linestrip.use();
		linestrip.uniform("RATIO", RATIO);
		linestripinstance.render(GL_LINE_STRIP, KTriangles);

	};

	auto upload = [&](){

		trianglebuf->fill(triangles);
		pointbuf->fill(points);
		pointmesh.SIZE = NPoints;

	};

	//doreset();

	// Main Functions

	Tiny::view.pipeline = [&](){

		Tiny::view.target(color::black);				//Target Main Screen

		doreset();
		draw();

	};

	int NN = 0;
	Tiny::loop([&](){

		if(paused) return;

		// Compute Cost and Gradients, Shift Points

		doenergy();
		doshift();

		// Retrieve Data from Compute Shader

		tenergybuf->retrieve(NTriangles, err);
		pointbuf->retrieve(points);

	//	if( geterr() < 1E-6 ){
		if(donext){
			donext = false;
			cout<<"RETRIANGULATE"<<endl;

			paused = true;

			if(!importlist.empty()){

				read(to_string(importlist.back())+".tri");
				importlist.pop_back();

				doreset();
				doenergy();

				draw();

			}

			else{

				Tiny::event.quit = true;
				write("out.tri");
				return;

			}

		}

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
