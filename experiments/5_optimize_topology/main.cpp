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

	Tiny::window("Energy Based Image Triangulation, Nicholas Mcdonald 2022", 600, 900);

	bool paused = true;

	Tiny::event.handler = [&](){

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_p)
			paused = !paused;

	};
	Tiny::view.interface = [](){};

	Texture tex(image::load("ingrid.png"));		//Load Texture with Image
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

	int n = 0;

	Tiny::view.pipeline = [&](){

		Tiny::view.target(color::black);				//Target Main Screen

		// Render Background Image

		image.use();														//Use Effect Shader
		image.texture("imageTexture", tex);		//Load Texture
		image.uniform("model", flat.model);		//Add Model Matrix
		flat.render();													//Render Primitive

		if(!paused){

			cout<<"NTRIANGLES "<<NTriangles<<endl;
			cout<<"NPOINTS "<<NPoints<<endl;

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
			average.dispatch(1 + NTriangles/1024);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			// Compute the Cost for every Triangle

			triangleshader.use();
			triangleshader.texture("imageTexture", tex);		//Load Texture
			triangleshader.uniform("mode", 1);
			triangleshader.uniform("KTriangles", KTriangles);
			triangleinstance.render(GL_TRIANGLE_STRIP);


			// Compute the Gradients (One per Triangle)

			gradient.use();
			gradient.uniform("K", KTriangles);
			gradient.dispatch(1 + KTriangles/1024);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			// Shift the Points

			shift.use();
			shift.uniform("NPoints", NPoints);
			shift.dispatch(1 + NPoints/1024);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);



			// Render Image to Screen

			triangleshader.use();
			triangleshader.texture("imageTexture", tex);		//Load Texture
			triangleshader.uniform("mode", 2);
			triangleshader.uniform("K", KTriangles);
			triangleinstance.render(GL_TRIANGLE_STRIP);

			point.use();
			pointmesh.render(GL_POINTS);

			linestrip.use();
			linestripinstance.render(GL_LINE_STRIP);




			// Retrieve Data

			tenergybuf.retrieve(NTriangles, err);
			tcolnumbuf.retrieve(NTriangles, cn);
			pointbuf->retrieve(points);			                            //Retrieve Data from Compute Shader

			// TOPOLOGICAL OPTIMIZATIONS

			// Execute Splits

			/*

			maxerr = 0;
			int tta = 0;
			for(size_t i = 0; i < KTriangles; i++){
				if(cn[i] == 0) continue;
				if(sqrt(err[i]) >= maxerr){
					maxerr = sqrt(err[i]);
					tta = i;
				}
			}


			if(cn[tta] > 100)
			if(split(tta)){

				*/

			maxerr = 0;
			int tta = -1;
			for(size_t i = 0; i < KTriangles; i++){
				if(cn[i] == 0) continue;
				if(cn[i] <= 100) continue;
				if(sqrt(err[i]) >= maxerr){
					maxerr = sqrt(err[i]);
					tta = i;
				}
			}

			if(tta >= 0)
			if(split(tta)){

				cout<<"FILL"<<endl;

				trianglebuf->fill(triangles);
				pointbuf->fill(points);

				cout<<"BIND"<<endl;

				triangleinstance.bind<ivec4>("in_Index", trianglebuf);
				triangleinstance.SIZE = KTriangles;
				linestripinstance.bind<ivec4>("in_Index", trianglebuf);
				linestripinstance.SIZE = KTriangles;
				pointmesh.SIZE = NPoints;

				cout<<"Do"<<endl;

				reset.use();
				reset.uniform("NTriangles", NTriangles);
				reset.uniform("NPoints", NPoints);

				cout<<"AYY"<<endl;

				if(NTriangles > NPoints) reset.dispatch(1 + NTriangles/1024);
				else reset.dispatch(1 + NPoints/1024);

				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

				cout<<"AYY"<<endl;

				// Accumulate Buffers

				triangleshader.use();
				triangleshader.texture("imageTexture", tex);		//Load Texture
				triangleshader.uniform("mode", 0);
				triangleshader.uniform("KTriangles", KTriangles);
				triangleinstance.render(GL_TRIANGLE_STRIP);

				cout<<"AYY"<<endl;

				// Average Accmulation Buffers, Compute Color!

				cout<<NTriangles<<endl;

				average.use();
				average.uniform("NTriangles", NTriangles);
				average.dispatch(1 + NTriangles/1024);
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

				cout<<"AYY"<<endl;

				// Compute the Cost for every Triangle

				triangleshader.use();
				triangleshader.texture("imageTexture", tex);		//Load Texture
				triangleshader.uniform("mode", 3);
				triangleshader.uniform("KTriangles", KTriangles);
				triangleinstance.render(GL_TRIANGLE_STRIP);

			}

			cout<<"DRAW"<<endl;

			triangleshader.use();
			triangleshader.texture("imageTexture", tex);		//Load Texture
			triangleshader.uniform("mode", 2);
			triangleshader.uniform("K", KTriangles);
			triangleinstance.render(GL_TRIANGLE_STRIP);

		//	point.use();
		//	pointmesh.render(GL_POINTS);

		//	linestrip.use();
		//	linestripinstance.render(GL_LINE_STRIP);




			if(optimize()){

				// Update Buffers

				trianglebuf->fill(triangles);
				pointbuf->fill(points);
				triangleinstance.bind<ivec4>("in_Index", trianglebuf);
				triangleinstance.SIZE = KTriangles;
				linestripinstance.bind<ivec4>("in_Index", trianglebuf);
				linestripinstance.SIZE = KTriangles;
				pointmesh.SIZE = NPoints;

			}

		}




	};

	Tiny::loop([&](){});

	delete[] err;
	delete[] cn;

	delete trianglebuf;
	delete pointbuf;

	Tiny::quit();

	return 0;
}
