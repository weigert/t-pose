#include <TinyEngine/TinyEngine>
#include <TinyEngine/image>
#include <TinyEngine/color>

//const float RATIO = 6.0f/9.0f;
float RATIO = 12.0f/6.75f;

#include "../../source/triangulate.h"

int main( int argc, char* args[] ) {

	Tiny::view.pointSize = 2.0f;
	Tiny::view.vsync = false;
	Tiny::view.antialias = 0;
//	Tiny::benchmark = true;

	Tiny::window("Energy Based Image Triangulation, Nicholas Mcdonald 2022", 960, 540);

	bool paused = true;

	Tiny::event.handler = [&](){

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_p)
			paused = !paused;

	};
	Tiny::view.interface = [](){};

	Texture tex(image::load("../../resource/imageA.png"));		//Load Texture with Image
	Square2D flat;														//Create Primitive Model
	Shader image({"shader/image.vs", "shader/image.fs"}, {"in_Quad", "in_Tex"});
	Shader point({"shader/point.vs", "shader/point.fs"}, {"in_Position"});

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	initialize( 0 );
	cout<<"Number of Triangles: "<<trianglebuf->SIZE<<endl;

	Triangle triangle;
	Instance triangleinstance(&triangle);
	triangleinstance.SIZE = KTriangles;
	//triangleinstance.bind<ivec4>("in_Index", trianglebuf);

	TLineStrip tlinestrip;
	Instance linestripinstance(&tlinestrip);
	linestripinstance.SIZE = KTriangles;
	//linestripinstance.bind<ivec4>("in_Index", trianglebuf);

	Shader triangleshader({"shader/triangle.vs", "shader/triangle.fs"}, {"in_Position"}, {"points", "index", "colacc", "colnum", "energy"});
	triangleshader.bind<vec2>("points", pointbuf);
	triangleshader.bind<ivec4>("index", trianglebuf);

	Shader linestrip({"shader/linestrip.vs", "shader/linestrip.fs"}, {"in_Position"}, {"points", "index"});
	linestrip.bind<vec2>("points", pointbuf);
	linestrip.bind<ivec4>("index", trianglebuf);

	Model pointmesh({"in_Position"});
	pointmesh.bind<vec2>("in_Position", pointbuf);
	pointmesh.SIZE = NPoints;

	// Color Accumulation Buffers

	triangleshader.bind<ivec4>("colacc", tcolaccbuf);
	triangleshader.bind<int>("colnum", tcolnumbuf);
	triangleshader.bind<int>("energy", tenergybuf);

	// SSBO Manipulation Compute Shaders (Reset / Average)

	Compute reset({"shader/reset.cs"}, {"colacc", "colnum", "energy", "gradient"});
	reset.bind<ivec4>("colacc", tcolaccbuf);
	reset.bind<int>("colnum", tcolnumbuf);
	reset.bind<int>("energy", tenergybuf);
	reset.bind<ivec2>("gradient", pgradbuf);

	Compute average({"shader/average.cs"}, {"colacc", "colnum", "energy"});
	average.bind<ivec4>("colacc", tcolaccbuf);
	average.bind<int>("colnum", tcolnumbuf);
	average.bind<int>("energy", tenergybuf);

	Compute gradient({"shader/gradient.cs"}, {"index", "energy", "gradient"});
	gradient.bind<ivec4>("index", trianglebuf);
	gradient.bind<int>("energy", tenergybuf);
	gradient.bind<ivec2>("gradient", pgradbuf);

	Compute shift({"shader/shift.cs"}, {"points", "gradient"});
	shift.bind<ivec4>("points", pointbuf);
	shift.bind<ivec2>("gradient", pgradbuf);

	// Convenience Lambdas

	auto computecolors = [&]( bool other = true ){

		reset.use();
		reset.uniform("NTriangles", NTriangles);
		reset.uniform("NPoints", NPoints);

		if(NTriangles > NPoints) reset.dispatch(1 + NTriangles/1024);
		else reset.dispatch(1 + NPoints/1024);

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);		//Load Texture
		triangleshader.uniform("mode", 0);
		triangleshader.uniform("KTriangles", KTriangles);
		triangleshader.uniform("RATIO", RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, NTriangles);

		average.use();
		average.uniform("NTriangles", NTriangles);
		average.dispatch(1 + NTriangles/1024);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	};

	auto doenergy = [&](){

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);
		triangleshader.uniform("mode", 1);
		triangleshader.uniform("KTriangles", KTriangles);
		triangleshader.uniform("RATIO", RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, NTriangles);

	};

	auto doshift = [&](){

		gradient.use();
		gradient.uniform("KTriangles", KTriangles);
		gradient.uniform("RATIO", RATIO);
		gradient.dispatch(1 + KTriangles/1024);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		shift.use();
		shift.uniform("NPoints", NPoints);
		shift.uniform("RATIO", RATIO);
		shift.dispatch(1 + NPoints/1024);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	};

	auto draw = [&](){

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);		//Load Texture
		triangleshader.uniform("mode", 2);
		triangleshader.uniform("K", KTriangles);
		triangleshader.uniform("RATIO", RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, KTriangles);

	//	point.use();
	//	point.uniform("RATIO", RATIO);
	//	pointmesh.render(GL_POINTS);

	//	linestrip.use();
	//	linestrip.uniform("RATIO", RATIO);
	//	linestripinstance.render(GL_LINE_STRIP, KTriangles);

	};

	auto upload = [&](){

		trianglebuf->fill(triangles);
		pointbuf->fill(points);
		pointmesh.SIZE = NPoints;

	};

	computecolors();

	// Main Functions

	Tiny::view.pipeline = [&](){

		Tiny::view.target(color::black);				//Target Main Screen

		computecolors();
		draw();

	};

	vector<int> exportlist = {
		1000,
		500,
		400,
		300,
		200,
		100,
		30
	};

	int NN = 0;
	Tiny::loop([&](){

		if(paused) return;

		// Compute Cost and Gradients, Shift Points

		doenergy();
		doshift();

		// Retrieve Data from Compute Shader

		tenergybuf->retrieve(NTriangles, err);
		tcolnumbuf->retrieve(NTriangles, cn);
		pointbuf->retrieve(points);

		// TOPOLOGICAL OPTIMIZATIONS

		bool updated = false;

		if( geterr() < 1E-3 ){

			// Make sure we start exportin'

			if(exportlist.empty()){
				Tiny::event.quit = true;
				return;
			}

			if(KTriangles >= exportlist.back()){
				write("triangulation_"+to_string(exportlist.back())+".tri");
				exportlist.pop_back();
			}

			int tta = maxerrid();
			if(tta >= 0)
			if(split(tta)){

				updated = true;

				// Necessary if we split more than one guy

				//upload();
				//computecolors( false );
				//doenergy( false );

			}

		}

		if(optimize())
			updated = true;

		if(updated){
			upload();
			cout<<"KTRIANGLES "<<KTriangles<<endl;
		}

	});

	delete[] err;
	delete[] cn;

	delete trianglebuf;
	delete pointbuf;
	delete tcolaccbuf;
	delete tcolnumbuf;
	delete tenergybuf;
	delete pgradbuf;

	Tiny::quit();

	return 0;
}
