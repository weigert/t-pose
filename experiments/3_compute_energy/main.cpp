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
	Square2D flat;														//Create Primitive Model
	Shader image({"shader/image.vs", "shader/image.fs"}, {"in_Quad", "in_Tex"});
	Shader point({"shader/point.vs", "shader/point.fs"}, {"in_Position"});

	initialize( 1024 );
	cout<<"Number of Triangles: "<<trianglebuf->SIZE<<endl;

	Triangle triangle;
	Instance triangleinstance(&triangle);
	triangleinstance.bind<ivec4>("in_Index", trianglebuf);
	triangleinstance.SIZE = KTriangles;

	Shader triangleshader({"shader/triangle.vs", "shader/triangle.fs"}, {"in_Position", "in_Index"}, {"points", "colacc", "colnum", "energy"});
	triangleshader.bind<vec2>("points", pointbuf);

	Model pointmesh({"in_Position"});
	pointmesh.bind<vec2>("in_Position", pointbuf);
	pointmesh.SIZE = pointbuf->SIZE;

	// Color Accumulation Buffers

	Buffer tcolaccbuf( trianglebuf->SIZE, (ivec4*)NULL );		// Raw Color
	Buffer tcolnumbuf( trianglebuf->SIZE, (int*)NULL );			// Triangle Size (Pixels)
	Buffer tenergybuf( trianglebuf->SIZE, (int*)NULL );			// Triangle Energy

	triangleshader.bind<ivec4>("colacc", &tcolaccbuf);
	triangleshader.bind<int>("colnum", &tcolnumbuf);
	triangleshader.bind<int>("energy", &tenergybuf);

	// SSBO Manipulation Compute Shaders (Reset / Average)

	Compute reset({"shader/reset.cs"}, {"colacc", "colnum", "energy"});
	reset.bind<ivec4>("colacc", &tcolaccbuf);
	reset.bind<int>("colnum", &tcolnumbuf);
	reset.bind<int>("energy", &tenergybuf);

	Compute average({"shader/average.cs"}, {"colacc", "colnum", "energy"});
	average.bind<ivec4>("colacc", &tcolaccbuf);
	average.bind<int>("colnum", &tcolnumbuf);
	average.bind<int>("energy", &tenergybuf);

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
		triangleshader.uniform("mode", 0);
		triangleinstance.render(GL_TRIANGLE_STRIP);

		// Average Accmulation Buffers, Compute Color!

		average.use();
		average.uniform("N", KTriangles);
		average.uniform("mode", 0);
		average.dispatch(1+KTriangles/1024);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// Compute the Cost for every Triangle

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);		//Load Texture
		triangleshader.uniform("mode", 1);
		triangleinstance.render(GL_TRIANGLE_STRIP);

		// Average the Energy

		average.use();
		average.uniform("N", KTriangles);
		average.uniform("mode", 1);
		average.dispatch(1+KTriangles/1024);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// Render the Triangles to Screen

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);		//Load Texture
		triangleshader.uniform("mode", 2);
		triangleinstance.render(GL_TRIANGLE_STRIP);

		// Render Points

		point.use();
		pointmesh.render(GL_POINTS);

	};

	Tiny::loop([&](){});
	Tiny::quit();

	return 0;
}
