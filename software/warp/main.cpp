#include <TinyEngine/TinyEngine>
#include <TinyEngine/image>
#include <TinyEngine/color>
#include <TinyEngine/parse>

#include <tpose/tpose>
#include <tpose/triangulation>
#include <tpose/io>

using namespace std;
using namespace glm;

int main( int argc, char* args[] ) {

	parse::get(argc, args);

	if(!parse::option.contains("ia")){
		cout<<"Please specify an input image A with -ia."<<endl;
		exit(0);
	}

	if(!parse::option.contains("ib")){
		cout<<"Please specify an input image B with -ib."<<endl;
		exit(0);
	}

	if(!parse::option.contains("ta")){
		cout<<"Please specify input triangulation A with -ta."<<endl;
		exit(0);
	}

	if(!parse::option.contains("tb")){
		cout<<"Please specify input triangulation B with -tb."<<endl;
		exit(0);
	}

	// Load Images, Triangulations

	SDL_Surface* IMGA = IMG_Load(parse::option["ia"].c_str());
	if(IMGA == NULL){
		cout<<"Failed to load image."<<endl;
		exit(0);
	}

	SDL_Surface* IMGB = IMG_Load(parse::option["ib"].c_str());
	if(IMGB == NULL){
		cout<<"Failed to load image."<<endl;
		exit(0);
	}

	if(IMGA->w != IMGB->w || IMGA->h != IMGB->h){
		cout<<"Images don't have the same dimension"<<endl;
		exit(0);
	}

	string triA = parse::option["ta"];
	string triB = parse::option["tb"];

	// Launch Window

	Tiny::view.vsync = false;
	Tiny::view.antialias = 0;

	Tiny::window("Energy Based Image Triangulation, Nicholas Mcdonald 2022", IMGA->w/1.5, IMGA->h/1.5);

	glDisable(GL_CULL_FACE);

	bool paused = true;
	bool viewlines = false;

	Tiny::event.handler = [&](){

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_p)
			paused = !paused;

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_n)
			viewlines = !viewlines;

	};
	Tiny::view.interface = [](){};

	tpose::init();

	// Shaders and Buffers

	Shader linestrip({"shader/linestrip.vs", "shader/linestrip.fs"}, {"in_Position"}, {"points", "index"});
	linestrip.bind<vec2>("points", tpose::pointbuf);
	linestrip.bind<ivec4>("index", tpose::trianglebuf);

	Shader triangleshader({"shader/triangle.vs", "shader/triangle.fs"}, {"in_Position"}, {"points", "index", "colacc", "colnum", "tenergy", "penergy", "gradient", "nring"});
	triangleshader.bind<vec2>("points", tpose::pointbuf);
	triangleshader.bind<ivec4>("index", tpose::trianglebuf);
	triangleshader.bind<ivec4>("colacc", tpose::tcolaccbuf);
	triangleshader.bind<int>("colnum", tpose::tcolnumbuf);
	triangleshader.bind<int>("tenergy", tpose::tenergybuf);
	triangleshader.bind<int>("penergy", tpose::penergybuf);
	triangleshader.bind<ivec2>("gradient", tpose::pgradbuf);
	triangleshader.bind<int>("nring", tpose::tnringbuf);

	Compute gradient({"shader/gradient.cs"}, {"index", "tenergy", "penergy", "gradient"});
	gradient.bind<ivec4>("index", tpose::trianglebuf);
	gradient.bind<int>("tenergy", tpose::tenergybuf);
	gradient.bind<int>("penergy", tpose::penergybuf);
	gradient.bind<ivec2>("gradient", tpose::pgradbuf);

	Compute shift({"shader/shift.cs"}, {"points", "gradient"});
	shift.bind<ivec4>("points", tpose::pointbuf);
	shift.bind<ivec2>("gradient", tpose::pgradbuf);

	// Load two Triangulations

	bool warpA = true;

	tpose::triangulation trA, trB;
	tpose::io::read(&trA, triA);
	tpose::io::read(&trB, triB);

	Texture texA(IMGA);		//Load Texture with Image A
	Texture texB(IMGB);		//Load Texture with Image B

	tpose::triangulation* tr;

	if(warpA) tr = &trA;
	else tr = &trB;

	tpose::upload(tr);

	// Render Objects

	Square2D flat;

	Triangle triangle;
	Instance triangleinstance(&triangle);

	TLineStrip tlinestrip;
	Instance linestripinstance(&tlinestrip);

	// Convenience Lambdas

	auto doreset = [&](){

		triangleshader.use();
		triangleshader.texture("imageA", texA);		//Load Texture
		triangleshader.texture("imageB", texB);		//Load Texture
		triangleshader.uniform("warpA", warpA);
		triangleshader.uniform("mode", 0);
		triangleshader.uniform("KTriangles", tr->NT);
		triangleshader.uniform("RATIO", tpose::RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, (13*tr->NT));

	};

	auto doenergy = [&](){

		triangleshader.use();
		triangleshader.texture("imageA", texA);		//Load Texture
		triangleshader.texture("imageB", texB);		//Load Texture
		triangleshader.uniform("warpA", warpA);
		triangleshader.uniform("mode", 1);
		triangleshader.uniform("KTriangles", tr->NT);
		triangleshader.uniform("RATIO", tpose::RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, (13*tr->NT));

	};

	auto doshift = [&](){

		gradient.use();
		gradient.uniform("KTriangles", tr->NT);
		gradient.uniform("RATIO", tpose::RATIO);
		gradient.dispatch(1 + tr->NT/1024);

		shift.use();
		shift.uniform("NPoints", tr->NP);
		shift.uniform("RATIO", tpose::RATIO);
		shift.dispatch(1 + tr->NP/1024);

	};

	auto draw = [&](){

		triangleshader.use();
		triangleshader.texture("imageA", texA);		//Load Texture
		triangleshader.texture("imageB", texB);		//Load Texture
		triangleshader.uniform("warpA", warpA);
		triangleshader.uniform("mode", 2);
		triangleshader.uniform("KTriangles", tr->NT);
		triangleshader.uniform("RATIO", tpose::RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, tr->NT);

		if(viewlines){

			linestrip.use();
			linestrip.uniform("RATIO", tpose::RATIO);
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

		tpose::tenergybuf->retrieve((13*tr->NT), tpose::terr);
		tpose::penergybuf->retrieve(tr->NP, tpose::perr);
		tpose::tcolnumbuf->retrieve((13*tr->NT), tpose::cn);
		tpose::pointbuf->retrieve(tr->points);

		if( tpose::geterr(tr) < 1E-6 ){

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

			if(NWARPA < 1 && NWARPB < 1){
				tpose::upload(tr);
				return;
			}

			cout<<"IM HERE"<<endl;

			// Re-Load Both

			NWARPA = 0; NWARPB = 0;

			tpose::io::write(&trA, triA+".warp");
			tpose::io::write(&trB, triB+".warp");

			if(!tpose::io::read(&trA, triA, true)){
				Tiny::event.quit = true;
				return;
			}

			if(!tpose::io::read(&trB, triB, true)){
				Tiny::event.quit = true;
				return;
			}

			tpose::upload(tr);

		}
			/*

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

			tpose::upload(tr);

		}

		*/

	});


	tpose::quit();
	Tiny::quit();

	return 0;
}
