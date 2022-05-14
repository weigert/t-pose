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

	/*

		Procedure:

			Generate an Initial Triangulation...

			1. Create Raw Vertices, Store in SSBO
			2. Create Initial Triangulation from Half-Edges, Edges

			Create Renderable Triangle Primitives

			3. Use an Index Buffer to Render the Triangles, No Depth Test!
			4. Use a geometry shader to create 12 triangles for each triangle,
					representing possible vertex shifts.
			5. Compute the total energy for each of the 12 triangles, storing it in an SSBO
			6. We can compute the gradient of a triangles energy for the given triangle, in dependency of the vertex.

			7. We can compute the gradient of the vertex energy.
			8. We can adjust the vertex positions and iterate

			Adjust the topology:

			9. Split triangles where appropriate,
			10. Merge triangles where appropriate using some check
			11. ???

			Use the initial triangulation and topology to warp the image to another one!
			Estimate pose!

	*/

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
