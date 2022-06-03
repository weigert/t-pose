#include <TinyEngine/TinyEngine>
#include <TinyEngine/image>
#include <TinyEngine/color>
#include <TinyEngine/parse>

#include <tpose/tpose>
#include <tpose/triangulation>
#include <tpose/io>

using namespace glm;
using namespace std;

int main( int argc, char* args[] ) {

	parse::get(argc, args);

	string tri;
	if(!parse::option.contains("t")){
		cout<<"Please specify a input triangulation with -t."<<endl;
		exit(0);
	}
	else tri = parse::option["t"];

	SDL_Surface* IMG = NULL;
	if(parse::option.contains("i")){
		IMG = IMG_Load(parse::option["i"].c_str());
		if(IMG == NULL) cout<<"Failed to load image "<<parse::option["i"]<<endl;
	}

	Tiny::view.pointSize = 2.0f;
	Tiny::view.vsync = false;
	Tiny::view.antialias = 0;

	tpose::triangulation trA;
	if(!tpose::io::read(&trA, tri))
		exit(0);

	Tiny::window("Energy Based Image Triangulation, Nicholas Mcdonald 2022", tpose::RATIO*600, 600);

	bool paused = true;
	bool showlines = false;

	Tiny::view.interface = [](){};

	Texture tex;
	Square2D flat;																						//Create Primitive Model
	if(IMG != NULL) tex.raw(IMG);		//Load Texture with Image

	glDisable(GL_CULL_FACE);

	tpose::init();

	Buffer otherpointbuf;

	Triangle triangle;
	Instance triangleinstance(&triangle);

	TLineStrip tlinestrip;
	Instance linestripinstance(&tlinestrip);

	Shader triangleshader({"shader/triangle.vs", "shader/triangle.fs"}, {"in_Position"}, {"points", "index", "colacc", "otherpoints"});
	triangleshader.bind<vec2>("points", tpose::pointbuf);
	triangleshader.bind<ivec4>("index", tpose::trianglebuf);
	triangleshader.bind<ivec4>("colacc", tpose::tcolaccbuf);
	triangleshader.bind<vec2>("otherpoints", &otherpointbuf);

	Shader linestrip({"shader/linestrip.vs", "shader/linestrip.fs"}, {"in_Position"}, {"points", "index", "otherpoints"});
	linestrip.bind<vec2>("points", tpose::pointbuf);
	linestrip.bind<ivec4>("index", tpose::trianglebuf);
	linestrip.bind<vec2>("otherpoints", &otherpointbuf);

	tpose::upload(&trA);
	otherpointbuf.fill(trA.originpoints);

	Tiny::event.handler = [&](){

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_p)
			paused = !paused;

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_n)
			showlines = !showlines;

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_SPACE){
			if(tpose::io::read(&trA, tri)){
				tpose::upload(&trA);
				otherpointbuf.fill(trA.originpoints);
			}
		}

	};

	// Color Accumulation Buffers

	float s = 0.0f;

	Tiny::view.pipeline = [&](){

		Tiny::view.target(color::black);				//Target Main Screen

		triangleshader.use();

		if(IMG != NULL)
		triangleshader.texture("imageTexture", tex);		//Load Texture
		triangleshader.uniform("mode", 2);
		triangleshader.uniform("KTriangles", trA.NT);
		triangleshader.uniform("s", s);
		triangleshader.uniform("RATIO", tpose::RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, trA.NT);

		if(showlines){

			linestrip.use();
			linestrip.uniform("RATIO", tpose::RATIO);
			linestrip.uniform("s", s);
			linestripinstance.render(GL_LINE_STRIP, trA.NT);

		}

	};

	int NN = 0;
	float ds = 0.001f;
	Tiny::loop([&](){
		s += ds;
		if(s < 0 || s > 1) ds *= -1;
	});

	tpose::quit();
	Tiny::quit();

	return 0;

}
