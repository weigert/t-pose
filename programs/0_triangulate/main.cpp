#include <TinyEngine/TinyEngine>
#include <TinyEngine/image>
#include <TinyEngine/color>
#include <TinyEngine/parse>

#include "../../source/triangulate.h"

#include <boost/filesystem.hpp>

using namespace std;
using namespace glm;

int main( int argc, char* args[] ) {

	// Load Parameters

	parse::get(argc, args);

	string outfolder;
	if(!parse::option.contains("o")){
		cout<<"Please specify an output folder with -o."<<endl;
		exit(0);
	}
	else{
		outfolder = parse::option["o"];
		if(!boost::filesystem::is_directory(boost::filesystem::current_path()/".."/".."/"output"/outfolder))
			boost::filesystem::create_directory(boost::filesystem::current_path()/".."/".."/"output"/outfolder);
	}

	SDL_Surface* IMG = NULL;
	if(!parse::option.contains("i")){
		cout<<"Please specify an input image with -i."<<endl;
		exit(0);
	}
	else IMG = IMG_Load(parse::option["i"].c_str());
	if(IMG == NULL){
		cout<<"Failed to load image."<<endl;
		exit(0);
	}

	// Setup Window

	Tiny::view.vsync = false;
	Tiny::view.antialias = 0;
	Tiny::window("Energy Based Image Triangulation, Nicholas Mcdonald 2022", IMG->w/1.5, IMG->h/1.5);
	glDisable(GL_CULL_FACE);

	tri::RATIO = (float)IMG->w/(float)IMG->h;

	bool paused = true;
	Tiny::view.interface = [](){};
	Tiny::event.handler = [&](){

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_p)
			paused = !paused;

	};

	// Image Rendering

	Texture tex(IMG);		//Load Texture with Image
	Square2D flat;			//Create Primitive Model

	Shader image({"shader/image.vs", "shader/image.fs"}, {"in_Quad", "in_Tex"});
	Shader point({"shader/point.vs", "shader/point.fs"}, {"in_Position"});

	// Shaders and Buffers

	tri::init();

	Shader linestrip({"shader/linestrip.vs", "shader/linestrip.fs"}, {"in_Position"}, {"points", "index"});
	linestrip.bind<vec2>("points", tri::pointbuf);
	linestrip.bind<ivec4>("index", tri::trianglebuf);

	Shader triangleshader({"shader/triangle.vs", "shader/triangle.fs"}, {"in_Position"}, {"points", "index", "colacc", "colnum", "tenergy", "penergy", "gradient", "nring"});
	triangleshader.bind<vec2>("points", tri::pointbuf);
	triangleshader.bind<ivec4>("index", tri::trianglebuf);
	triangleshader.bind<ivec4>("colacc", tri::tcolaccbuf);
	triangleshader.bind<int>("colnum", tri::tcolnumbuf);
	triangleshader.bind<int>("tenergy", tri::tenergybuf);
	triangleshader.bind<int>("penergy", tri::penergybuf);
	triangleshader.bind<ivec2>("gradient", tri::pgradbuf);
	triangleshader.bind<int>("nring", tri::tnringbuf);

	// SSBO Manipulation Compute Shaders (Reset / Average)

	Compute reset({"shader/reset.cs"}, {"colacc", "colnum", "tenergy", "penergy", "gradient", "nring"});
	reset.bind<ivec4>("colacc", tri::tcolaccbuf);
	reset.bind<int>("colnum", tri::tcolnumbuf);
	reset.bind<int>("tenergy", tri::tenergybuf);
	reset.bind<int>("penergy", tri::penergybuf);
	reset.bind<ivec2>("gradient", tri::pgradbuf);
	reset.bind<int>("nring", tri::tnringbuf);

	Compute gradient({"shader/gradient.cs"}, {"index", "tenergy", "penergy", "gradient"});
	gradient.bind<ivec4>("index", tri::trianglebuf);
	gradient.bind<int>("tenergy", tri::tenergybuf);
	gradient.bind<int>("penergy", tri::penergybuf);
	gradient.bind<ivec2>("gradient", tri::pgradbuf);

	Compute shift({"shader/shift.cs"}, {"points", "gradient"});
	shift.bind<ivec4>("points", tri::pointbuf);
	shift.bind<ivec2>("gradient", tri::pgradbuf);

	// Triangulation and Models

	tri::triangulation tr;
	tri::upload(&tr, false);

	cout<<"Number of Triangles: "<<tr.NT<<endl;

	Triangle triangle;
	Instance triangleinstance(&triangle);

	TLineStrip tlinestrip;
	Instance linestripinstance(&tlinestrip);

	Model pointmesh({"in_Position"});
	pointmesh.bind<vec2>("in_Position", tri::pointbuf);

	// Convenience Lambdas

	auto computecolors = [&](){

		reset.use();
		reset.uniform("NTriangles", 13*tr.NT);
		reset.uniform("NPoints", tr.NP);
		reset.dispatch(1 + (13*tr.NT)/1024);

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);		//Load Texture
		triangleshader.uniform("mode", 0);
		triangleshader.uniform("KTriangles", tr.NT);
		triangleshader.uniform("RATIO", tri::RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, (13*tr.NT));

	};

	auto doenergy = [&](){

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);
		triangleshader.uniform("mode", 1);
		triangleshader.uniform("KTriangles", tr.NT);
		triangleshader.uniform("RATIO", tri::RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, (13*tr.NT));

	};

	auto doshift = [&](){

		gradient.use();
		gradient.uniform("KTriangles", tr.NT);
		gradient.uniform("RATIO", tri::RATIO);
		gradient.dispatch(1 + tr.NT/1024);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		shift.use();
		shift.uniform("NPoints", tr.NP);
		shift.uniform("RATIO", tri::RATIO);
		shift.dispatch(1 + tr.NP/1024);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	};

	auto draw = [&](){

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);		//Load Texture
		triangleshader.uniform("mode", 2);
		triangleshader.uniform("KTriangles", tr.NT);
		triangleshader.uniform("RATIO", tri::RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, tr.NT);

	//	point.use();
	//	point.uniform("RATIO", tri::RATIO);
	//	pointmesh.render(GL_POINTS, tr.NT);

	//	linestrip.use();
	//	linestrip.uniform("RATIO", tri::RATIO);
	//p	linestripinstance.render(GL_LINE_STRIP, tr.NT);

	};

	computecolors();

	// Main Functions

	Tiny::view.pipeline = [&](){

		Tiny::view.target(color::black);				//Target Main Screen

		computecolors();
		draw();

	};

	vector<int> exportlist = {
		1500,
		1400,
		1300,
		1200,
		1100,
		1000,
		900,
		800,
		700,
		600,
		500,
		400,
		300,
		200,
		100,
		50
	};

	Tiny::loop([&](){

		if(paused) return;

		// Compute Cost and Gradients, Shift Points

		doenergy();
		doshift();

		// Retrieve Data from Compute Shader

		tri::tenergybuf->retrieve((13*tr.NT), tri::terr);
		tri::penergybuf->retrieve((13*tr.NT), tri::perr);
		tri::tcolnumbuf->retrieve((13*tr.NT), tri::cn);
		tri::pointbuf->retrieve(tr.points);

		// TOPOLOGICAL OPTIMIZATIONS

		bool updated = false;

		if( tri::geterr(&tr) < 1E-4 ){

			// Make sure we start exportin'

			if(exportlist.empty()){
				Tiny::event.quit = true;
				return;
			}

			if(tr.NT >= exportlist.back()){
				tri::tcolaccbuf->retrieve(tr.NT, &tr.colors[0]);
				tr.write("../../output/"+outfolder+"/"+to_string(exportlist.back())+".tri");
				exportlist.pop_back();
			}

			int tta = tri::maxerrid(&tr);
			float curmaxerr = tri::maxerr;

			while(tta >= 0 && tr.split(tta)){

				cout<<"NT: "<<tr.NT<<endl;

				updated = true;
				break;

				/*

				tr.optimize();

				tri::upload( &tr, false );
				computecolors();
				doenergy();

				tri::tenergybuf->retrieve((13*tr.NT), tri::terr);
				tri::penergybuf->retrieve((13*tr.NT), tri::perr);
				tri::tcolnumbuf->retrieve((13*tr.NT), tri::cn);
				tri::pointbuf->retrieve(tr.points);
				tta = tri::maxerrid(&tr);


				if(tta == -1) break;
				if(tri::maxerr <= curmaxerr)
					break;

					*/

			//		cout<<tri::maxerr<<" "<<curmaxerr<<endl;
			//	curmaxerr = tri::maxerr;

				// Necessary if we split more than one guy



			}

		}

		if(tr.optimize())
			updated = true;

		if(updated)
			tri::upload(&tr, false);

	});

	tri::quit();
	Tiny::quit();

	return 0;
}
