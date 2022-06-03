#include <TinyEngine/TinyEngine>
#include <TinyEngine/image>
#include <TinyEngine/color>

#include <tpose/tpose>
#include <tpose/triangulation>

using namespace std;
using namespace glm;

int main( int argc, char* args[] ) {

	Tiny::view.pointSize = 2.0f;
	Tiny::view.vsync = false;
	Tiny::view.antialias = 0;

	Tiny::window("Energy Based Image Triangulation, Nicholas Mcdonald 2022", 1200, 800);
	Tiny::event.handler = [](){};
	Tiny::view.interface = [](){};

	tpose::RATIO = 1.2/0.8;

	Texture tex(image::load("canyon.png"));		//Load Texture with Image
	Square2D flat;																						//Create Primitive Model

	Shader image({"shader/image.vs", "shader/image.fs"}, {"in_Quad", "in_Tex"});
	Shader point({"shader/point.vs", "shader/point.fs"}, {"in_Position"});

	glDisable(GL_CULL_FACE);
//	glDisable(GL_DEPTH_TEST);

	tpose::init();
	tpose::triangulation tr(1024);
	cout<<"Number of Triangles: "<<tr.NT<<endl;
	tpose::upload(&tr, false);

	Triangle triangle;
	Instance triangleinstance(&triangle);
	triangleinstance.bind<ivec4>("in_Index", tpose::trianglebuf);

	Shader triangleshader({"shader/triangle.vs", "shader/triangle.gs", "shader/triangle.fs"}, {"in_Position", "in_Index"}, {"points", "colacc", "colnum", "energy"});
	triangleshader.bind<vec2>("points", tpose::pointbuf);
	triangleshader.bind<ivec4>("colacc", tpose::tcolaccbuf);
	triangleshader.bind<int>("colnum", tpose::tcolnumbuf);
	triangleshader.bind<int>("energy", tpose::tenergybuf);

	// SSBO Manipulation Compute Shaders (Reset / Average)

	Compute reset({"shader/reset.cs"}, {"colacc", "colnum", "energy", "gradient"});
	reset.bind<ivec4>("colacc", tpose::tcolaccbuf);
	reset.bind<int>("colnum", tpose::tcolnumbuf);
	reset.bind<int>("energy", tpose::tenergybuf);
	reset.bind<ivec2>("gradient", tpose::pgradbuf);

	Compute average({"shader/average.cs"}, {"colacc", "colnum", "energy"});
	average.bind<ivec4>("colacc", tpose::tcolaccbuf);
	average.bind<int>("colnum", tpose::tcolnumbuf);
	average.bind<int>("energy", tpose::tenergybuf);

	Compute gradient({"shader/gradient.cs"}, {"energy", "gradient", "indices"});
	gradient.bind<int>("energy", tpose::tenergybuf);
	gradient.bind<ivec2>("gradient", tpose::pgradbuf);
	gradient.bind<ivec4>("indices", tpose::trianglebuf);

	Compute shift({"shader/shift.cs"}, {"points", "gradient"});
	shift.bind<ivec4>("points", tpose::pointbuf);
	shift.bind<ivec2>("gradient", tpose::pgradbuf);

	Model pointmesh({"in_Position"});
	pointmesh.bind<vec2>("in_Position", tpose::pointbuf);
	pointmesh.SIZE = tr.NP;

	Tiny::view.pipeline = [&](){

		Tiny::view.target(color::black);				//Target Main Screen

		// Render Background Image

		image.use();														//Use Effect Shader
		image.texture("imageTexture", tex);		//Load Texture
		image.uniform("model", flat.model);		//Add Model Matrix
		flat.render();													//Render Primitive

		// Reset Accumulation Buffers

		reset.use();
		reset.uniform("NTriangles", (13*tr.NT));
		reset.uniform("NPoints", tr.NP);

		if((13*tr.NT) > tr.NP) reset.dispatch(1 + (13*tr.NT)/1024);
		else reset.dispatch(1 + tr.NP/1024);

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// Accumulate Buffers

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);		//Load Texture
		triangleshader.uniform("mode", 0);
		triangleshader.uniform("KTriangles", tr.NT);
		triangleshader.uniform("RATIO", tpose::RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, tr.NT);

		// Average Accmulation Buffers, Compute Color!

		average.use();
		average.uniform("NTriangles", (13*tr.NT));
		average.uniform("mode", 0);
		average.dispatch(1 + (13*tr.NT)/1024);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// Compute the Cost for every Triangle

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);		//Load Texture
		triangleshader.uniform("mode", 1);
		triangleshader.uniform("KTriangles", tr.NT);
		triangleshader.uniform("RATIO", tpose::RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, (13*tr.NT));

		// Compute the Gradients (One per Triangle)

		gradient.use();
		gradient.uniform("K", tr.NT);
		gradient.uniform("mode", 0);
		gradient.dispatch(1 + tr.NT/1024);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// Shift the Points

		shift.use();
		shift.uniform("NPoints", tr.NP);
		shift.uniform("mode", 0);
		shift.uniform("RATIO", tpose::RATIO);
		shift.dispatch(1 + tr.NP/1024);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// Render Points

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);		//Load Texture
		triangleshader.uniform("mode", 3);
		triangleshader.uniform("KTriangles", tr.NT);
		triangleshader.uniform("RATIO", tpose::RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, tr.NT);

		point.use();
		point.uniform("RATIO", tpose::RATIO);
		pointmesh.render(GL_POINTS);

	};

	Tiny::loop([&](){});

	tpose::quit();
	Tiny::quit();

	return 0;
}
