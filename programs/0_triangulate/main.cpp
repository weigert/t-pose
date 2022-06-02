#include <TinyEngine/TinyEngine>
#include <TinyEngine/image>
#include <TinyEngine/color>
#include <TinyEngine/parse>

#include "../../source/triangulate.h"

#include <boost/filesystem.hpp>
#include <set>
#include <map>
#include <iomanip>

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

	Tiny::window("Energy Based Image Triangulation, Nicholas Mcdonald 2022", IMG->w, IMG->h);
	tri::RATIO = (float)IMG->w/(float)IMG->h;

	glDisable(GL_CULL_FACE);

	bool paused = true;
	bool viewlines = false;

	Tiny::view.interface = [](){};
	Tiny::event.handler = [&](){

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_p)
			paused = !paused;

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_n)
		viewlines = !viewlines;

	};

	// Image Rendering

	Texture tex(IMG);		//Load Texture with Image

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

	Compute gradient({"shader/gradient.cs"}, {"index", "tenergy", "gradient"});
	gradient.bind<ivec4>("index", tri::trianglebuf);
	gradient.bind<int>("tenergy", tri::tenergybuf);
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

		shift.use();
		shift.uniform("NPoints", tr.NP);
		shift.uniform("RATIO", tri::RATIO);
		shift.dispatch(1 + tr.NP/1024);

	};

	auto draw = [&](){

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);		//Load Texture
		triangleshader.uniform("mode", 2);
		triangleshader.uniform("KTriangles", tr.NT);
		triangleshader.uniform("RATIO", tri::RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, tr.NT);

		if(viewlines){

			linestrip.use();
			linestrip.uniform("RATIO", tri::RATIO);
			linestripinstance.render(GL_LINE_STRIP, tr.NT);

		}

	};

	computecolors();

	// Main Functions

	Tiny::view.pipeline = [&](){

		Tiny::view.target(color::black);				//Target Main Screen

		computecolors();
		draw();

	};

	vector<int> exportlist = {
		4000,
		3000,
		2000,
		1500,
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

			struct setsort {
		    bool operator () (const std::pair<int, float>& lhs, const std::pair<int, float>& rhs) const {
		        return lhs.second > rhs.second;
		     }
		  };

		  std::set<std::pair<int, float>, setsort> hset;
		  for(int t = 0; t < tr.triangles.size(); t++){

		    if( tr.halfedges[ 3*t + 0 ] >= 0 )
		      hset.emplace( 3*t + 0, tri::terr[t] + tri::terr[tr.halfedges[ 3*t + 0 ]/3] );
		    if( tr.halfedges[ 3*t + 1 ] >= 0 )
		      hset.emplace( 3*t + 1, tri::terr[t] + tri::terr[tr.halfedges[ 3*t + 1 ]/3] );
		    if( tr.halfedges[ 3*t + 2 ] >= 0 )
		      hset.emplace( 3*t + 2, tri::terr[t] + tri::terr[tr.halfedges[ 3*t + 2 ]/3] );

		  }

		  std::set<int> nflipset;         // Halfedges NOT to flip!
		  std::map<int, float> hflipset;  // Halfedges to Flip

			// Iterate over the sorted triangles

		  for(auto& h: hset){

		    if(nflipset.contains(h.first))  // Non-Flip Halfedge
		      continue;

		    if( tr.halfedges[h.first] < 0 ) // No Opposing Halfedge
		      continue;

		    if(nflipset.contains(tr.halfedges[h.first]))  // Opposing Half-Edge Not Flippable
		      continue;

		    // Compute Energy for this Half-Edge Pair

		    hflipset[ h.first ] = h.second;

		    // Add all Half-Edges of Both Triangles to the Non-Flip Set

		    int ta = h.first/3;
		    int tb = tr.halfedges[ h.first ]/3;

		    nflipset.emplace( 3*ta + 0 );
		    nflipset.emplace( 3*ta + 1 );
		    nflipset.emplace( 3*ta + 2 );
		    nflipset.emplace( 3*tb + 0 );
		    nflipset.emplace( 3*tb + 1 );
		    nflipset.emplace( 3*tb + 2 );

		  }

		  // Flip the flip set, then check the energy, and flip those that don't work back!

		  for(auto& h: hflipset)
		    tr.flip( h.first, 0.0f );

		  tri::upload(&tr, false);
		  computecolors();
		  doenergy();

		  tri::tenergybuf->retrieve((13*tr.NT), tri::terr);

		  for(auto& h: hflipset){

		    if( tri::terr[ h.first/3 ] + tri::terr[ (tr.halfedges[ h.first ])/3 ] > h.second )
		      tr.flip( h.first, 0.0f ); 	//Flip it Back, Split

		  }

		  tri::upload(&tr, false);
		  computecolors();
		  doenergy();
		  tri::tenergybuf->retrieve((13*tr.NT), tri::terr);

		  int tta = tri::maxerrid(&tr);
		  if(tta >= 0 && tr.split(tta))
		    updated = true;

		}

		// Prune Flat Boundary Triangles

		for(size_t ta = 0; ta < tr.NT; ta++)
		if(tr.boundary(ta) == 3)
		  if(tr.prune(ta)) updated = true;

		// Attempt a Delaunay Flip on a Triangle's Largest Angle

		for(size_t ta = 0; ta < tr.NT; ta++){

		  if(tr.angle( 3*ta + 0 ) > 0.8*tri::PI)
		    tr.flip( 3*ta + 0, 0.0 );
		  if(tr.angle( 3*ta + 1 ) > 0.8*tri::PI)
		    tr.flip( 3*ta + 1, 0.0 );
		  if(tr.angle( 3*ta + 2 ) > 0.8*tri::PI)
		    tr.flip( 3*ta + 2, 0.0 );

		}

		// Collapse Small Edges

		for(size_t ta = 0; ta < tr.triangles.size(); ta++){

		  int ha = 3*ta + 0;
		  float minlength = tr.hlength( ha );
		  if(tr.hlength( ha + 1 ) < minlength)
		    minlength = tr.hlength( ++ha );
		  if(tr.hlength( ha + 1 ) < minlength)
		    minlength = tr.hlength( ++ha );
		  if(tr.collapse(ha))
		    updated = true;

		}

		if(updated){
			cout<<tr.NT<<" "<<std::setprecision(16)<<gettoterr( &tr )<<endl;
			tri::upload(&tr, false);
		}

	});

	tri::quit();
	Tiny::quit();

	return 0;
}
