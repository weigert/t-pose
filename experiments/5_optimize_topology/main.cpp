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

	initialize( 2048 );
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
	int KTriangles = trianglebuf->SIZE;
	int NTriangles = (1+12)*KTriangles;

	// Color Accumulation Buffers

	Buffer tcolaccbuf( 2*NTriangles, (ivec4*)NULL );		// Raw Color
	Buffer tcolnumbuf( 2*NTriangles, (int*)NULL );			// Triangle Size (Pixels)
	Buffer tenergybuf( 2*NTriangles, (int*)NULL );			// Triangle Energy
	Buffer pgradbuf( 2*NPoints, (ivec2*)NULL );

	triangleshader.bind<ivec4>("colacc", &tcolaccbuf);
	triangleshader.bind<int>("colnum", &tcolnumbuf);
	triangleshader.bind<int>("energy", &tenergybuf);

	int* err = new int[2*NTriangles];
	int* cn = new int[2*NTriangles];

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


		// TOPOLOGICAL OPTIMIZATIONS

		/*

				Topological Optimization:
					- Collapse Vertices (Thin Triangles)
					- Delaunay Flip (Flat Triangles)
					- Centroid Split (Bad Triangles)

		*/

		// Retrieve Data

		tenergybuf.retrieve(tenergybuf.SIZE, err);
		tcolnumbuf.retrieve(tcolnumbuf.SIZE, cn);
		pointbuf->retrieve(points);			                            //Retrieve Data from Compute Shader

		// Delaunay Flip

		for(size_t j = 0; j < 50; j++){

		int ta = rand()%KTriangles;					// Pick a Random Triangle
		int ha = 3*ta + rand()%3;						// Random Half-Edge Index
		int hb = halfedges[3*ta + ha%3];							// Neighboring Half-Edge
		int tb = hb/3;											// Neighboring Triangle

	//	cout<<"Triangle "<<ta<<" "<<tb<<endl;
	//	cout<<"Halfedge "<<ha<<" "<<hb<<endl;

		if(hb >= 0){

			// Compute Sum of Angles

			vec2 paa = points[triangles[ta][(ha+0)%3]];
			vec2 pab = points[triangles[ta][(ha+1)%3]];
			vec2 pac = points[triangles[ta][(ha+2)%3]];
			float wa = acos(dot(paa - pac, pab - pac) / length(paa - pac) / length(pab - pac));

			vec2 pba = points[triangles[tb][(hb+0)%3]];
			vec2 pbb = points[triangles[tb][(hb+1)%3]];
			vec2 pbc = points[triangles[tb][(hb+2)%3]];
			float wb = acos(dot(pba - pbc, pbb - pbc) / length(pba - pbc) / length(pbb - pbc));

		//	cout<<"Angles "<<wa<<" "<<wb<<endl;

			if(wa + wb >= 3.14159265f){


				int ta0 = halfedges[3*ta+(ha+0)%3];	//h3
				int ta1 = halfedges[3*ta+(ha+1)%3];	//h6
				int ta2 = halfedges[3*ta+(ha+2)%3];	//h7

				int tb0 = halfedges[3*tb+(hb+0)%3];	//h0
				int tb1 = halfedges[3*tb+(hb+1)%3];	//h8
				int tb2 = halfedges[3*tb+(hb+2)%3];	//h9

				//if(ta0 != -1 && ta1 != -1 && ta2 != -1)
				//if(tb0 != -1 && tb1 != -1 && tb2 != -1){

					cout<<"PREFLIP, "<<ha<<" "<<hb<<endl;

					ivec4 tca = triangles[ta];
					ivec4 tcb = triangles[tb];

					cout<<tca.x<<" "<<tca.y<<" "<<tca.z<<endl;
					cout<<tcb.x<<" "<<tcb.y<<" "<<tcb.z<<endl;

					cout<<ta0<<" "<<ta1<<" "<<ta2<<endl;
					cout<<tb0<<" "<<tb1<<" "<<tb2<<endl;

					// Internal Shift

					halfedges[3*ta+(ha+0)%3] = ta0;
					halfedges[3*ta+(ha+1)%3] = ta2;
					halfedges[3*ta+(ha+2)%3] = tb1;

					halfedges[3*tb+(hb+0)%3] = tb0;
					halfedges[3*tb+(hb+1)%3] = tb2;
					halfedges[3*tb+(hb+2)%3] = ta1;

					// External Shift

					halfedges[ta1] = 3*tb+(hb+2)%3;
					halfedges[ta2] = 3*ta+(ha+1)%3;

					halfedges[tb1] = 3*ta+(ha+2)%3;
					halfedges[tb2] = 3*tb+(hb+1)%3;



					triangles[ta][(ha+0)%3] = tcb[(hb+2)%3];
					triangles[ta][(ha+1)%3] = tca[(ha+2)%3];
					triangles[ta][(ha+2)%3] = tcb[(hb+1)%3];

					triangles[tb][(hb+0)%3] = tca[(ha+2)%3];
					triangles[tb][(hb+1)%3] = tcb[(hb+2)%3];
					triangles[tb][(hb+2)%3] = tca[(ha+1)%3];

					cout<<"POSTFLIP"<<endl;
					tca = triangles[ta];
					tcb = triangles[tb];

					cout<<tca.x<<" "<<tca.y<<" "<<tca.z<<endl;
					cout<<tcb.x<<" "<<tcb.y<<" "<<tcb.z<<endl;

					trianglebuf->fill(triangles);


				}








	//		}

		}

	}



		//

		/*

		cout<<n++<<endl;
		if(n%50 != 0)
			return;

		// Find the guy with the biggest error!

	//	int t = rand()%KTriangles;

		int maxerr = 0;
		for(size_t i = 0; i < KTriangles; i++){
			if(cn[i] == 0) continue;
			if(sqrt(err[i]) >= maxerr)
				maxerr = sqrt(err[i]);
		}


		cout<<"MAXERR "<<maxerr<<endl;
		if(maxerr <= 10)
			return;

		for(size_t t = 0; t < KTriangles; t++){

			if(cn[t] == 0) continue;
			if(sqrt(err[t]) < 0.9*maxerr)
				continue;

			if(cn[t] < 15)
				continue;

			cout<<"ERR "<<t<<" "<<sqrt(err[t])<<" "<<cn[t]<<endl;

			// Split the Triangle

			ivec4 tind = triangles[t];

			vec2 npos = (points[tind.x] + points[tind.y] + points[tind.z])/3.0f;
			int nind = points.size();
			points.push_back(npos);

			// Replace 3 Triangles

			triangles.push_back(ivec4(nind, tind.y, tind.z, 0));
			triangles.push_back(ivec4(tind.x, nind, tind.z, 0));
			triangles[t].z = nind;

			// Update Numbers

			KTriangles += 2;
			NTriangles = (1+12)*KTriangles;
			NPoints++;

		}


		// Update Buffers

		trianglebuf->fill(triangles);
		pointbuf->fill(points);

		pointmesh.SIZE = pointbuf->SIZE;
		triangleinstance.SIZE = trianglebuf->SIZE;

		*/

	};



	Tiny::loop([&](){});

	Tiny::quit();

	return 0;
}
