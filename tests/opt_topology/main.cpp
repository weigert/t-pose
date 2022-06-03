#include <TinyEngine/TinyEngine>
#include <TinyEngine/image>
#include <TinyEngine/color>

#include <tpose/tpose>
#include <tpose/triangulation>
#include <tpose/io>

using namespace std;
using namespace glm;

int main( int argc, char* args[] ) {

	Tiny::view.pointSize = 2.0f;
	Tiny::view.vsync = false;
	Tiny::view.antialias = 0;

	Tiny::window("Energy Based Image Triangulation, Nicholas Mcdonald 2022", 900, 600);

	tpose::RATIO= 9.0/6.0;

	bool paused = true;

	Tiny::event.handler = [&](){

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_p)
			paused = !paused;

	};
	Tiny::view.interface = [](){};

	Texture tex(image::load("fruit.png"));		//Load Texture with Image
	Square2D flat;																					//Create Primitive Model

	Shader image({"shader/image.vs", "shader/image.fs"}, {"in_Quad", "in_Tex"});
	Shader point({"shader/point.vs", "shader/point.fs"}, {"in_Position"});

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	tpose::init();
	tpose::triangulation tr;
	cout<<"Number of Triangles: "<<tr.NT<<endl;
	tpose::upload(&tr, false);

	Triangle triangle;
	Instance triangleinstance(&triangle);
	triangleinstance.bind<ivec4>("in_Index", tpose::trianglebuf);

	TLineStrip tlinestrip;
	Instance linestripinstance(&tlinestrip);
	linestripinstance.bind<ivec4>("in_Index", tpose::trianglebuf);

	Shader triangleshader({"shader/triangle.vs", "shader/triangle.gs", "shader/triangle.fs"}, {"in_Position", "in_Index"}, {"points", "colacc", "colnum", "energy"});
	triangleshader.bind<vec2>("points", tpose::pointbuf);
	triangleshader.bind<ivec4>("colacc", tpose::tcolaccbuf);
	triangleshader.bind<int>("colnum", tpose::tcolnumbuf);
	triangleshader.bind<int>("energy", tpose::tenergybuf);

	Shader linestrip({"shader/linestrip.vs", "shader/linestrip.fs"}, {"in_Position", "in_Index"}, {"points"});
	linestrip.bind<vec2>("points", tpose::pointbuf);

	// SSBO Manipulation Compute Shaders (Reset / Average)

	Compute reset({"shader/reset.cs"}, {"colacc", "colnum", "energy", "gradient"});
	reset.bind<ivec4>("colacc", tpose::tcolaccbuf);
	reset.bind<int>("colnum", tpose::tcolnumbuf);
	reset.bind<int>("energy", tpose::tenergybuf);
	reset.bind<ivec2>("gradient", tpose::pgradbuf);

	Compute average({"shader/average.cs"}, {"colacc", "colnum", "energy"});
	average.bind<ivec4>("colacc", tpose::tcolaccbuf);
	average.bind<int>("colnum", tpose::tcolnumbuf);
	average.bind<int>("energy", tpose::tenergybuf);

	Compute gradient({"shader/gradient.cs"}, {"energy", "gradient", "indices"});
	gradient.bind<int>("energy", tpose::tenergybuf);
	gradient.bind<ivec2>("gradient", tpose::pgradbuf);
	gradient.bind<ivec4>("indices", tpose::trianglebuf);

	Compute shift({"shader/shift.cs"}, {"points", "gradient"});
	shift.bind<ivec4>("points", tpose::pointbuf);
	shift.bind<ivec2>("gradient", tpose::pgradbuf);

	Model pointmesh({"in_Position"});
	pointmesh.bind<vec2>("in_Position", tpose::pointbuf);
	pointmesh.SIZE = tr.NP;

	// Convenience Lambdas

	auto computecolors = [&]( bool other = true ){

		reset.use();
		reset.uniform("NTriangles", (13*tr.NT));
		reset.uniform("NPoints", tr.NP);

		if((13*tr.NT) > tr.NP) reset.dispatch(1 + (13*tr.NT)/1024);
		else reset.dispatch(1 + tr.NP/1024);

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);		//Load Texture
		triangleshader.uniform("mode", 0);
		triangleshader.uniform("KTriangles", tr.NT);
		triangleshader.uniform("drawother", other);
		triangleshader.uniform("RATIO", tpose::RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, (13*tr.NT));

		average.use();
		average.uniform("NTriangles", (13*tr.NT));
		average.dispatch(1 + (13*tr.NT)/1024);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	};

	auto doenergy = [&](){

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);
		triangleshader.uniform("mode", 1);
		triangleshader.uniform("KTriangles", tr.NT);
		triangleshader.uniform("drawother", true);
		triangleshader.uniform("RATIO", tpose::RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, (13*tr.NT));

	};

	auto doshift = [&](){

		gradient.use();
		gradient.uniform("K", tr.NT);
		gradient.uniform("RATIO", tpose::RATIO);
		gradient.dispatch(1 + tr.NT/1024);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		shift.use();
		shift.uniform("NPoints", tr.NP);
		shift.uniform("RATIO", tpose::RATIO);
		shift.dispatch(1 + tr.NP/1024);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	};

	auto draw = [&](){

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);		//Load Texture
		triangleshader.uniform("mode", 2);
		triangleshader.uniform("drawother", false);
		triangleshader.uniform("K", tr.NT);
		triangleshader.uniform("RATIO", tpose::RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, tr.NT);

	//	point.use();
	//	point.uniform("RATIO", RATIO);
	//	pointmesh.render(GL_POINTS);

		linestrip.use();
		linestrip.uniform("RATIO", tpose::RATIO);
		linestripinstance.render(GL_LINE_STRIP, tr.NT);

	};

	computecolors();

	// Main Functions

	Tiny::view.pipeline = [&](){

		Tiny::view.target(color::black);				//Target Main Screen

		computecolors();
		draw();

	};

	Tiny::loop([&](){

		if(paused) return;

		// Compute Cost and Gradients, Shift Points

		doenergy();
		doshift();

		// Retrieve Data from Compute Shader

		tpose::tenergybuf->retrieve((13*tr.NT), tpose::terr);
		tpose::penergybuf->retrieve((13*tr.NT), tpose::perr);
		tpose::tcolnumbuf->retrieve((13*tr.NT), tpose::cn);
		tpose::pointbuf->retrieve(tr.points);

		// TOPOLOGICAL OPTIMIZATIONS

		bool updated = false;

		if( tpose::geterr(&tr) < 1E-3 ){

			int tta = tpose::maxerrid(&tr);
			if(tta >= 0)
			if(tr.split(tta))
				updated = true;

		}

		if(tr.optimize())
			updated = true;

		if(updated)
			tpose::upload(&tr, false);

	});

	tpose::quit();
	Tiny::quit();

	return 0;
}
