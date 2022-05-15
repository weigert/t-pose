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
	
	Tiny::window("Energy Based Image Triangulation, Nicholas Mcdonald 2022", 1200, 800);
	Tiny::event.handler = [](){};
	Tiny::view.interface = [](){};

	Texture tex(image::load("canyon.png"));		//Load Texture with Image
	Square2D flat;														//Create Primitive Model
	Shader image({"shader/image.vs", "shader/image.fs"}, {"in_Quad", "in_Tex"});
	Shader point({"shader/point.vs", "shader/point.fs"}, {"in_Position"});

	initialize();
	cout<<"Number of Triangles: "<<trianglebuf->SIZE<<endl;

	Triangle triangle;
	Instance triangleinstance(&triangle);
	triangleinstance.bind<vec3>("in_Index", trianglebuf);

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

	/*
	compute.bind<float>("buff", &buf);							    //Upload the Buffer
  compute.use();																        //Use the Shader

  int K = 256;                                          //K-Ary Merge
  compute.uniform("K", K);                              //Set Uniform

  std::cout<<"Parallel ";
  timer::benchmark<std::chrono::milliseconds>([&](){

  for(int rest = size%K, stride = size/K;
      stride >= 1 || rest > 0;
      rest = stride%K, stride /= K){

    compute.uniform("rest", rest);                      //Set Uniforms
    compute.uniform("stride", stride);                  //
    compute.dispatch(1+stride/1024);                    //Round-Up Division

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  }

  buf.retrieve(buffer);

	*/

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

	Tiny::view.pipeline = [&](){

		Tiny::view.target(color::black);				//Target Main Screen

		// Render Background Image

		image.use();														//Use Effect Shader
		image.texture("imageTexture", tex);		//Load Texture
		image.uniform("model", flat.model);		//Add Model Matrix
		flat.render();													//Render Primitive

		// Reset Accumulation Buffers

		reset.use();
		reset.uniform("N", (int)trianglebuf->SIZE);
		reset.dispatch(1+trianglebuf->SIZE/1024);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// Accumulate Buffers

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);		//Load Texture
		triangleshader.uniform("renderaverage", false);
		triangleinstance.render(GL_TRIANGLE_STRIP);

		// Average Accmulation Buffers, Compute Color!

		average.use();
		average.uniform("N", (int)trianglebuf->SIZE);
		average.dispatch(1+trianglebuf->SIZE/1024);
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

	float t = 0; //Time

	FastNoiseLite noise;
	noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	noise.SetFractalType(FastNoiseLite::FractalType_FBm);
	noise.SetFractalOctaves(8.0f);
	noise.SetFractalLacunarity(2.0f);
	noise.SetFractalGain(0.6f);
	noise.SetFrequency(1.0);

	Tiny::loop([&](){ //Execute every frame

		t += 0.005;

		for(unsigned int i = 0; i < points.size(); i++){
			offset[i].x = points[i].x + 0.5f*0.1*noise.GetNoise(points[i].x, points[i].y, t);
			offset[i].y = points[i].y + 0.5f*0.1*noise.GetNoise(points[i].x, points[i].y, -t);
		}

		pointbuf->fill<vec2>(offset);

	});

	Tiny::quit();

	return 0;
}
