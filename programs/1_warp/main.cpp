#include <TinyEngine/TinyEngine>
#include <TinyEngine/image>
#include <TinyEngine/color>
#include <TinyEngine/parse>

#include "../../source/triangulate.h"
#include "../../source/io.h"

#include <boost/filesystem.hpp>

using namespace std;
using namespace glm;

int main( int argc, char* args[] ) {

	parse::get(argc, args);

	string outa;
	if(!parse::option.contains("oa")){
		cout<<"Please specify output folder A with -oa."<<endl;
		exit(0);
	}
	else{
		outa = parse::option["oa"];
		if(!boost::filesystem::is_directory(boost::filesystem::current_path()/".."/".."/"output"/outa))
			boost::filesystem::create_directory(boost::filesystem::current_path()/".."/".."/"output"/outa);
	}

	string outb;
	if(!parse::option.contains("ob")){
		cout<<"Please specify output folder B with -ob."<<endl;
		exit(0);
	}
	else{
		outb = parse::option["ob"];
		if(!boost::filesystem::is_directory(boost::filesystem::current_path()/".."/".."/"output"/outb))
			boost::filesystem::create_directory(boost::filesystem::current_path()/".."/".."/"output"/outb);
	}

	SDL_Surface* IMGA = NULL;
	if(!parse::option.contains("ia")){
		cout<<"Please specify an input image A with -ia."<<endl;
		exit(0);
	}
	else IMGA = IMG_Load(parse::option["ia"].c_str());
	if(IMGA == NULL){
		cout<<"Failed to load image."<<endl;
		exit(0);
	}

	SDL_Surface* IMGB = NULL;
	if(!parse::option.contains("ib")){
		cout<<"Please specify an input image B with -ib."<<endl;
		exit(0);
	}
	else IMGB = IMG_Load(parse::option["ib"].c_str());
	if(IMGB == NULL){
		cout<<"Failed to load image."<<endl;
		exit(0);
	}

	if(IMGA->w != IMGB->w || IMGA->h != IMGB->h){
		cout<<"Images don't have the same dimension"<<endl;
		exit(0);
	}

	Tiny::view.vsync = false;
	Tiny::view.antialias = 0;
	Tiny::window("Energy Based Image Triangulation, Nicholas Mcdonald 2022", IMGA->w/1.5, IMGA->h/1.5);
	glDisable(GL_CULL_FACE);

	tri::RATIO = (float)IMGA->w/(float)IMGA->h;

	bool paused = true;
	bool viewlines = false;

	Tiny::event.handler = [&](){

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_p)
			paused = !paused;

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_n)
			viewlines = !viewlines;

	};
	Tiny::view.interface = [](){};

	tri::init();

	// Shaders and Buffers

	Shader image({"shader/image.vs", "shader/image.fs"}, {"in_Quad", "in_Tex"});
	Shader point({"shader/point.vs", "shader/point.fs"}, {"in_Position"});

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

	// Load Triangulations

	vector<int> importlist = {
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

	// Load two Triangulations

	bool warpA = true;

	tri::triangulation trA("../../output/"+outa+"/"+to_string(importlist.back())+".tri");
	tri::triangulation trB("../../output/"+outb+"/"+to_string(importlist.back())+".tri");

	Texture texA(IMGA);		//Load Texture with Image A
	Texture texB(IMGB);		//Load Texture with Image B

	tri::triangulation* tr;

	if(warpA) tr = &trA;
	else tr = &trB;

	tri::upload(tr);

	// Render Objects

	Square2D flat;

	Triangle triangle;
	Instance triangleinstance(&triangle);

	TLineStrip tlinestrip;
	Instance linestripinstance(&tlinestrip);

	// Convenience Lambdas

	auto doreset = [&](){

		reset.use();
		reset.uniform("NTriangles", 13*tr->NT);
		reset.uniform("NPoints", tr->NP);
		reset.dispatch(1 + (13*tr->NT)/1024);

		triangleshader.use();
		triangleshader.texture("imageA", texA);		//Load Texture
		triangleshader.texture("imageB", texB);		//Load Texture
		triangleshader.uniform("warpA", warpA);
		triangleshader.uniform("mode", 0);
		triangleshader.uniform("KTriangles", tr->NT);
		triangleshader.uniform("RATIO", tri::RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, (13*tr->NT));

	};

	auto doenergy = [&](){

		triangleshader.use();
		triangleshader.texture("imageA", texA);		//Load Texture
		triangleshader.texture("imageB", texB);		//Load Texture
		triangleshader.uniform("warpA", warpA);
		triangleshader.uniform("mode", 1);
		triangleshader.uniform("KTriangles", tr->NT);
		triangleshader.uniform("RATIO", tri::RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, (13*tr->NT));

	};

	auto doshift = [&](){

		gradient.use();
		gradient.uniform("KTriangles", tr->NT);
		gradient.uniform("RATIO", tri::RATIO);
		gradient.dispatch(1 + tr->NT/1024);

		shift.use();
		shift.uniform("NPoints", tr->NP);
		shift.uniform("RATIO", tri::RATIO);
		shift.dispatch(1 + tr->NP/1024);

	};

	auto draw = [&](){

		triangleshader.use();
		triangleshader.texture("imageA", texA);		//Load Texture
		triangleshader.texture("imageB", texB);		//Load Texture
		triangleshader.uniform("warpA", warpA);
		triangleshader.uniform("mode", 2);
		triangleshader.uniform("KTriangles", tr->NT);
		triangleshader.uniform("RATIO", tri::RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, tr->NT);

		if(viewlines){
			linestrip.use();
			linestrip.uniform("RATIO", tri::RATIO);
			linestripinstance.render(GL_LINE_STRIP, tr->NT);
		}

	};

	// Main Functions
	doreset();

	Tiny::view.pipeline = [&](){

		Tiny::view.target(color::black);				//Target Main Screen
		draw();

	};

	int NWARPA = 0;
	int NWARPB = 0;

	Tiny::loop([&](){

		if(paused) return;

		// Compute Cost and Gradients, Shift Points

		doreset();
		doenergy();
		doshift();

		// Retrieve Data from Compute Shader

		tri::tenergybuf->retrieve((13*tr->NT), tri::terr);
		tri::penergybuf->retrieve(tr->NP, tri::perr);
		tri::tcolnumbuf->retrieve((13*tr->NT), tri::cn);
		tri::pointbuf->retrieve(tr->points);

		if( tri::geterr(tr) < 1E-6 ){

			// Flip the Warp

			if(warpA){

				trB.points = trB.originpoints;
				trA.reversewarp( trB.points );
				NWARPA++;

			}

			else {

				trA.points = trA.originpoints;
				trB.reversewarp( trA.points );
				NWARPB++;

			}

			warpA = !warpA;

			if(warpA) tr = &trA;
			else tr = &trB;

			// Check for Warp Count

			if(NWARPA < 3 && NWARPB < 3){
				tri::upload(tr);
				return;
			}

			// Re-Load Both

			NWARPA = 0; NWARPB = 0;

			// Finished: Export

			if(importlist.empty()){

				Tiny::event.quit = true;

				trA.write("../../output/"+outa+"/"+to_string(importlist.back())+".warp.tri", false);
//				io::writeenergy(&trA, "../../output/"+outa+"/energy"+to_string(importlist.back())+".txt");

				trB.write("../../output/"+outb+"/"+to_string(importlist.back())+".warp.tri", false);
//				io::writeenergy(&trB, "../../output/"+outb+"/energy"+to_string(importlist.back())+".txt");

				return;

			}

			// Output

			trA.write("../../output/"+outa+"/"+to_string(importlist.back())+".warp.tri", false);
//			io::writeenergy(&trA, "../../output/"+outa+"/energy"+to_string(importlist.back())+".txt");

			trB.write("../../output/"+outb+"/"+to_string(importlist.back())+".warp.tri", false);
//			io::writeenergy(&trB, "../../output/"+outb+"/energy"+to_string(importlist.back())+".txt");

			importlist.pop_back();

			trA.read("../../output/"+outa+"/"+to_string(importlist.back())+".tri");
			trB.read("../../output/"+outb+"/"+to_string(importlist.back())+".tri");

			tri::upload(tr);

		}

	});


	tri::quit();
	Tiny::quit();

	return 0;
}
