#include <TinyEngine/TinyEngine>
#include <TinyEngine/image>
#include <TinyEngine/color>

#include "FastNoiseLite.h"
#include "delaunator-cpp/delaunator-header-only.hpp"
#include "poisson.h"
#include "triangulate.h"

int main( int argc, char* args[] ) {

	Tiny::view.pointSize = 2.0f;
	Tiny::view.vsync = false;
	Tiny::view.antialias = 0;

	Tiny::window("Energy Based Image Triangulation, Nicholas Mcdonald 2022", 1200, 800);
	Tiny::event.handler = [](){};
	Tiny::view.interface = [](){};

	Texture tex(image::load("canyon.png"));		//Load Texture with Image
	Square2D flat;														//Create Primitive Model
	Shader image({"shader/image.vs", "shader/image.fs"}, {"in_Quad", "in_Tex"});
	Shader point({"shader/point.vs", "shader/point.fs"}, {"in_Position"});

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	initialize( 3000 );
	cout<<"Number of Triangles: "<<trianglebuf->SIZE<<endl;

	Triangle triangle;
	Instance triangleinstance(&triangle);
	triangleinstance.bind<ivec4>("in_Index", trianglebuf);

	Shader triangleshader({"shader/triangle.vs", "shader/triangle.gs", "shader/triangle.fs"}, {"in_Position", "in_Index"}, {"points", "colacc", "colnum", "energy"});
	triangleshader.bind<vec2>("points", pointbuf);

	Model pointmesh({"in_Position"});
	pointmesh.bind<vec2>("in_Position", pointbuf);
	pointmesh.SIZE = pointbuf->SIZE;

	int NPoints = pointbuf->SIZE;
	int NTriangles = (1+12)*trianglebuf->SIZE;
	int KTriangles = trianglebuf->SIZE;

	// Color Accumulation Buffers

	Buffer tcolaccbuf( NTriangles, (ivec4*)NULL );		// Raw Color
	Buffer tcolnumbuf( NTriangles, (int*)NULL );			// Triangle Size (Pixels)
	Buffer tenergybuf( NTriangles, (int*)NULL );			// Triangle Energy
	Buffer pgradbuf( NPoints, (ivec2*)NULL );

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

	Tiny::view.pipeline = [&](){

		Tiny::view.target(color::black);				//Target Main Screen

		// Render Background Image

		image.use();														//Use Effect Shader
		image.texture("imageTexture", tex);		//Load Texture
		image.uniform("model", flat.model);		//Add Model Matrix
		flat.render();													//Render Primitive

		// Reset Accumulation Buffers

		reset.use();
		reset.uniform("NTriangles", NTriangles);
		reset.uniform("NPoints", NPoints);

		if(NTriangles > NPoints) reset.dispatch(1 + NTriangles/1024);
		else reset.dispatch(1 + NPoints/1024);

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// Accumulate Buffers

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);		//Load Texture
		triangleshader.uniform("mode", 0);
		triangleshader.uniform("KTriangles", KTriangles);
		triangleinstance.render(GL_TRIANGLE_STRIP);

		// Average Accmulation Buffers, Compute Color!

		average.use();
		average.uniform("NTriangles", NTriangles);
		average.uniform("mode", 0);
		average.dispatch(1 + NTriangles/1024);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// Compute the Cost for every Triangle

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);		//Load Texture
		triangleshader.uniform("mode", 1);
		triangleshader.uniform("KTriangles", KTriangles);
		triangleinstance.render(GL_TRIANGLE_STRIP);

		// Average the Energy

		average.use();
		average.uniform("NTriangles", NTriangles);
		average.uniform("mode", 1);
		average.dispatch(1 + NTriangles/1024);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// Compute the Gradients (One per Triangle)

		gradient.use();
		gradient.uniform("K", KTriangles);
		gradient.uniform("mode", 0);
		gradient.dispatch(1 + KTriangles/1024);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// Shift the Points

		shift.use();
		shift.uniform("NPoints", NPoints);
		shift.uniform("mode", 0);
		shift.dispatch(1 + NPoints/1024);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// Render Points

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);		//Load Texture
		triangleshader.uniform("mode", 3);
		triangleshader.uniform("K", KTriangles);
		triangleinstance.render(GL_TRIANGLE_STRIP);

		point.use();
		pointmesh.render(GL_POINTS);

	};

	int n = 0;

	Tiny::loop([&](){
		cout<<n++<<endl;
	});

	Tiny::quit();

	return 0;
}
