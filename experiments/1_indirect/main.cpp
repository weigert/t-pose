#include <TinyEngine/TinyEngine>
#include <TinyEngine/image>
#include <TinyEngine/color>

#include "../../source/include/delaunator-cpp/delaunator-header-only.hpp"
#include "../../source/include/poisson.h"
#include "triangulate.h"

int main( int argc, char* args[] ) {

	Tiny::view.pointSize = 2.0f;

	Tiny::window("Energy Based Image Triangulation, Nicholas Mcdonald 2022", 1200, 800);
	Tiny::event.handler = [](){};
	Tiny::view.interface = [](){};

	Texture tex(image::load("../../resource/canyon.png"));		//Load Texture with Image
	Square2D flat;														//Create Primitive Model
	Shader image({"shader/image.vs", "shader/image.fs"}, {"in_Quad", "in_Tex"});
	Shader point({"shader/point.vs", "shader/point.fs"}, {"in_Position"});

	initialize();

	Triangle triangle;
	Instance triangleinstance(&triangle);
	triangleinstance.bind<vec3>("in_Index", trianglebuf);

	Shader triangleshader({"shader/triangle.vs", "shader/triangle.fs"}, {"in_Position", "in_Index"}, {"points"});
	triangleshader.bind<vec2>("points", pointbuf);

	Model pointmesh({"in_Position"});
	pointmesh.bind<vec2>("in_Position", pointbuf);
	pointmesh.SIZE = pointbuf->SIZE;

	Tiny::view.pipeline = [&](){

		Tiny::view.target(color::black);				//Target Main Screen

		image.use();														//Use Effect Shader
		image.texture("imageTexture", tex);		//Load Texture
		image.uniform("model", flat.model);		//Add Model Matrix
		flat.render();													//Render Primitive

		point.use();
		pointmesh.render(GL_POINTS);

		triangleshader.use();
		triangleinstance.render(GL_LINE_STRIP);
		//triangleinstance.render(GL_TRIANGLE_STRIP);

	};

	Tiny::loop([&](){});
	Tiny::quit();

	return 0;
}
