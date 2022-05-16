#include <TinyEngine/TinyEngine>
#include <TinyEngine/image>
#include <TinyEngine/color>

const float RATIO = 12.0f/8.0f;

#include "../../source/triangulate.h"

int main( int argc, char* args[] ) {

	Tiny::view.pointSize = 2.0f;
	Tiny::view.vsync = false;

	Tiny::window("Energy Based Image Triangulation, Nicholas Mcdonald 2022", 1200, 800);
	Tiny::event.handler = [](){};
	Tiny::view.interface = [](){};

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	Texture tex(image::load("../../resource/canyon.png"));		//Load Texture with Image
	Square2D flat;																						//Create Primitive Model

	Shader image({"shader/image.vs", "shader/image.fs"}, {"in_Quad", "in_Tex"});
	Shader point({"shader/point.vs", "shader/point.fs"}, {"in_Position"});

	initialize( 1024 );
	cout<<"Number of Triangles: "<<trianglebuf->SIZE<<endl;

	Triangle triangle;
	Instance triangleinstance(&triangle);
	triangleinstance.bind<ivec4>("in_Index", trianglebuf);

	Shader triangleshader({"shader/triangle.vs", "shader/triangle.fs"}, {"in_Position", "in_Index"}, {"points", "colacc", "colnum"});
	triangleshader.bind<vec2>("points", pointbuf);

	Model pointmesh({"in_Position"});
	pointmesh.bind<vec2>("in_Position", pointbuf);
	pointmesh.SIZE = pointbuf->SIZE;

	// Color Accumulation Buffers

	Buffer tcolaccbuf( trianglebuf->SIZE, (ivec4*)NULL );		// Raw Color
	Buffer tcolnumbuf( trianglebuf->SIZE, (int*)NULL );	// Number
  triangleshader.bind<ivec4>("colacc", &tcolaccbuf);
	triangleshader.bind<int>("colnum", &tcolnumbuf);

	// SSBO Manipulation Compute Shaders (Reset / Average)

	Compute reset({"shader/reset.cs"}, {"colacc", "colnum"});
	reset.bind<ivec4>("colacc", &tcolaccbuf);
	reset.bind<int>("colnum", &tcolnumbuf);

	Compute average({"shader/average.cs"}, {"colacc", "colnum"});
	average.bind<ivec4>("colacc", &tcolaccbuf);
	average.bind<int>("colnum", &tcolnumbuf);

	Tiny::view.pipeline = [&](){

		Tiny::view.target(color::black);				//Target Main Screen

		// Render Background Image

		image.use();														//Use Effect Shader
		image.texture("imageTexture", tex);		//Load Texture
		image.uniform("model", flat.model);		//Add Model Matrix
		flat.render();													//Render Primitive

		// Reset Accumulation Buffers

		reset.use();
		reset.uniform("N", KTriangles);
		reset.dispatch(1+KTriangles/1024);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// Accumulate Buffers

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);		//Load Texture
		triangleshader.uniform("renderaverage", false);
		triangleinstance.render(GL_TRIANGLE_STRIP);

		// Average Accmulation Buffers, Compute Color!

		average.use();
		average.uniform("N", KTriangles);
		average.dispatch(1+KTriangles/1024);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// Render with Average Color!

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);		//Load Texture
		triangleshader.uniform("renderaverage", true);
		triangleinstance.render(GL_TRIANGLE_STRIP);

		// Render Points

		point.use();
		pointmesh.render(GL_POINTS);

	};

	Tiny::loop([&](){});
	Tiny::quit();

	return 0;
}
