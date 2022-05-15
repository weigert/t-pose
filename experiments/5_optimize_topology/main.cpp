#include <TinyEngine/TinyEngine>
#include <TinyEngine/image>
#include <TinyEngine/color>

//const float RATIO = 6.0f/9.0f;
const float RATIO = 12.0f/8.0f;

#include "FastNoiseLite.h"
#include "delaunator-cpp/delaunator-header-only.hpp"
#include "poisson.h"
#include "triangulate.h"

int main( int argc, char* args[] ) {

	Tiny::view.pointSize = 2.0f;
	Tiny::view.vsync = false;
	Tiny::view.antialias = 0;

	Tiny::window("Energy Based Image Triangulation, Nicholas Mcdonald 2022", 900, 600);

	bool paused = true;

	Tiny::event.handler = [&](){

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_p)
			paused = !paused;

	};
	Tiny::view.interface = [](){};

	Texture tex(image::load("canyon.png"));		//Load Texture with Image
	Square2D flat;														//Create Primitive Model
	Shader image({"shader/image.vs", "shader/image.fs"}, {"in_Quad", "in_Tex"});
	Shader point({"shader/point.vs", "shader/point.fs"}, {"in_Position"});

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	initialize( 32 );
	cout<<"Number of Triangles: "<<trianglebuf->SIZE<<endl;

	Triangle triangle;
	Instance triangleinstance(&triangle);
	triangleinstance.bind<ivec4>("in_Index", trianglebuf);

	TLineStrip tlinestrip;
	Instance linestripinstance(&tlinestrip);
	linestripinstance.bind<ivec4>("in_Index", trianglebuf);

	Shader triangleshader({"shader/triangle.vs", "shader/triangle.gs", "shader/triangle.fs"}, {"in_Position", "in_Index"}, {"points", "colacc", "colnum", "energy"});
	triangleshader.bind<vec2>("points", pointbuf);

	Shader linestrip({"shader/linestrip.vs", "shader/linestrip.fs"}, {"in_Position", "in_Index"}, {"points"});
	linestrip.bind<vec2>("points", pointbuf);

	Model pointmesh({"in_Position"});
	pointmesh.bind<vec2>("in_Position", pointbuf);
	pointmesh.SIZE = NPoints;

	// Color Accumulation Buffers

	Buffer tcolaccbuf( 16*MAXTriangles, (ivec4*)NULL );		// Raw Color
	Buffer tcolnumbuf( 16*MAXTriangles, (int*)NULL );			// Triangle Size (Pixels)
	Buffer tenergybuf( 16*MAXTriangles, (int*)NULL );			// Triangle Energy
	Buffer pgradbuf( 16*MAXTriangles, (ivec2*)NULL );

	triangleshader.bind<ivec4>("colacc", &tcolaccbuf);
	triangleshader.bind<int>("colnum", &tcolnumbuf);
	triangleshader.bind<int>("energy", &tenergybuf);

	// SSBO Manipulation Compute Shaders (Reset / Average)

	Compute reset({"shader/reset.cs"}, {"colacc", "colnum", "energy", "gradient"});
	reset.bind<ivec4>("colacc", &tcolaccbuf);
	reset.bind<int>("colnum", &tcolnumbuf);
	reset.bind<int>("energy", &tenergybuf);
	reset.bind<ivec2>("gradient", &pgradbuf);

	Compute average({"shader/average.cs"}, {"colacc", "colnum", "energy"});
	average.bind<ivec4>("colacc", &tcolaccbuf);
	average.bind<int>("colnum", &tcolnumbuf);
	average.bind<int>("energy", &tenergybuf);

	Compute gradient({"shader/gradient.cs"}, {"energy", "gradient", "indices"});
	gradient.bind<int>("energy", &tenergybuf);
	gradient.bind<ivec2>("gradient", &pgradbuf);
	gradient.bind<ivec4>("indices", trianglebuf);

	Compute shift({"shader/shift.cs"}, {"points", "gradient"});
	shift.bind<ivec4>("points", pointbuf);
	shift.bind<ivec2>("gradient", &pgradbuf);

	// Convenience Lambdas

	auto computecolors = [&](){

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
		triangleinstance.render(GL_TRIANGLE_STRIP);

		average.use();
		average.uniform("NTriangles", NTriangles);
		average.dispatch(1 + NTriangles/1024);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	};

	auto doshift = [&](){

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);
		triangleshader.uniform("mode", 1);
		triangleshader.uniform("KTriangles", KTriangles);
		triangleshader.uniform("RATIO", RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP);

		gradient.use();
		gradient.uniform("K", KTriangles);
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
		triangleinstance.render(GL_TRIANGLE_STRIP);

		point.use();
		point.uniform("RATIO", RATIO);
		pointmesh.render(GL_POINTS);

		linestrip.use();
		linestrip.uniform("RATIO", RATIO);
		linestripinstance.render(GL_LINE_STRIP);

	};

	computecolors();

	Tiny::view.pipeline = [&](){

		Tiny::view.target(color::black);				//Target Main Screen

		computecolors();
		draw();

	};

	Tiny::loop([&](){

		if(paused) return;

		// Compute Cost and Gradients, Shift Points

		doshift();

		// Retrieve Data from Compute Shader

		tenergybuf.retrieve(NTriangles, err);
		tcolnumbuf.retrieve(NTriangles, cn);
		pointbuf->retrieve(points);

		// TOPOLOGICAL OPTIMIZATIONS

		if( geterr() < 1E-3 ){

			int tta = maxerrid();
			if(tta >= 0)
			if(split(tta)){

				// Update Buffers

				trianglebuf->fill(triangles);
				pointbuf->fill(points);
				triangleinstance.bind<ivec4>("in_Index", trianglebuf);
				linestripinstance.bind<ivec4>("in_Index", trianglebuf);
				triangleinstance.SIZE = KTriangles;
				linestripinstance.SIZE = KTriangles;
				pointmesh.SIZE = NPoints;

				computecolors();

				triangleshader.use();
				triangleshader.texture("imageTexture", tex);		//Load Texture
				triangleshader.uniform("mode", 3);
				triangleshader.uniform("KTriangles", KTriangles);
				triangleshader.uniform("RATIO", RATIO);
				triangleinstance.render(GL_TRIANGLE_STRIP);

			}

		}

		if(optimize()){

			// Update Buffers

			trianglebuf->fill(triangles);
			pointbuf->fill(points);
			triangleinstance.bind<ivec4>("in_Index", trianglebuf);
			linestripinstance.bind<ivec4>("in_Index", trianglebuf);
			triangleinstance.SIZE = KTriangles;
			linestripinstance.SIZE = KTriangles;
			pointmesh.SIZE = NPoints;

		}

	});

	delete[] err;
	delete[] cn;

	delete trianglebuf;
	delete pointbuf;

	Tiny::quit();

	return 0;
}
