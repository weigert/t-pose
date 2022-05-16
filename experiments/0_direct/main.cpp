#include <TinyEngine/TinyEngine>
#include <TinyEngine/image>
#include <TinyEngine/color>

#include "delaunator-cpp/delaunator-header-only.hpp"
#include "poisson.h"
#include "triangulate.h"

int main( int argc, char* args[] ) {

	Tiny::view.pointSize = 2.0f;

	Tiny::window("Energy Based Image Triangulation, Nicholas Mcdonald 2022", 1200, 800);
	Tiny::event.handler = [](){};
	Tiny::view.interface = [](){};

	Texture tex(image::load("canyon.png"));		//Load Texture with Image
	Square2D flat;														//Create Primitive Model
	Shader image({"shader/image.vs", "shader/image.fs"}, {"in_Quad", "in_Tex"});
	Shader point({"shader/point.vs", "shader/point.fs"}, {"in_Position"});

	// Triangulation

	Triangulation triangulation(2048);

	// The triangulation needs to be

	Tiny::view.pipeline = [&](){

		Tiny::view.target(color::black);				//Target Main Screen

		image.use();														//Use Effect Shader
		image.texture("imageTexture", tex);		//Load Texture
		image.uniform("model", flat.model);		//Add Model Matrix
		flat.render();													//Render Primitive

		point.use();
		triangulation.render(GL_POINTS);
		triangulation.render(GL_LINES);

	};

	Tiny::loop([&](){});
	Tiny::quit();

	return 0;
}
