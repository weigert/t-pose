#include <TinyEngine/TinyEngine>
#include <TinyEngine/image>
#include <TinyEngine/color>

#include "../../source/triangulate.h"
#include "../../source/unproject.h"
#include "../../source/io.h"

#include <boost/filesystem.hpp>

using namespace std;
using namespace glm;
using namespace Eigen;

int main( int argc, char* args[] ) {

	if(argc < 2){
		cout<<"Please specify an input folder."<<endl;
		exit(0);
	}
	else{

		string outfolder = args[1];
		if(!boost::filesystem::is_directory(boost::filesystem::current_path()/".."/".."/"output"/outfolder)){
			cout<<"Not a directory"<<endl;
			exit(0);
		}

	}

	string outfolder = args[1];

	Tiny::view.pointSize = 2.0f;
	Tiny::view.vsync = false;
	Tiny::view.antialias = 0;

	Tiny::window("Energy Based Triangulation Warping, Nicholas Mcdonald 2022", 960, 540);
	tri::RATIO = 9.6/5.4;

	bool paused = true;
	bool donext = false;

	Tiny::event.handler = [&](){

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_p)
			paused = !paused;

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_n)
			donext = true;

	};
	Tiny::view.interface = [](){};

	Texture tex(image::load("../../resource/imageB.png"));		//Load Texture with Image
	Square2D flat;																						//Create Primitive Model

	vector<int> importlist = {
		2000,
		1900,
		1800,
		1700,
		1600,
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

	Shader image({"shader/image.vs", "shader/image.fs"}, {"in_Quad", "in_Tex"});
	Shader point({"shader/point.vs", "shader/point.fs"}, {"in_Position"});

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	// Shaders and Buffers

	tri::init();


	Buffer lbuf;


	// Triangulation and Models

	Shader linestrip({"shader/linestrip.vs", "shader/linestrip.fs"}, {"in_Position"}, {"points", "index"});
	linestrip.bind<vec2>("points", tri::pointbuf);
	linestrip.bind<ivec4>("index", tri::trianglebuf);

	Shader triangleshader({"shader/triangle.vs", "shader/triangle.fs"}, {"in_Position"}, {"points", "index", "colacc", "colnum", "tenergy", "penergy", "gradient", "nring", "eline"});
	triangleshader.bind<vec2>("points", tri::pointbuf);
	triangleshader.bind<ivec4>("index", tri::trianglebuf);
	triangleshader.bind<ivec4>("colacc", tri::tcolaccbuf);
	triangleshader.bind<int>("colnum", tri::tcolnumbuf);
	triangleshader.bind<int>("tenergy", tri::tenergybuf);
	triangleshader.bind<int>("penergy", tri::penergybuf);
	triangleshader.bind<ivec2>("gradient", tri::pgradbuf);
	triangleshader.bind<int>("nring", tri::tnringbuf);
	triangleshader.bind<vec4>("eline", &lbuf);

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

	tri::triangulation tr("../../output/"+outfolder+"/"+to_string(importlist.back())+".tri");
	//tri::triangulation tr(1024);
	cout<<"Number of Triangles: "<<tr.NT<<endl;
	tri::upload(&tr);

	importlist.pop_back();

	Triangle triangle;
	Instance triangleinstance(&triangle);

	TLineStrip tlinestrip;
	Instance linestripinstance(&tlinestrip);

	Model pointmesh({"in_Position"});
	pointmesh.bind<vec2>("in_Position", tri::pointbuf);
	pointmesh.SIZE = tr.NP;





	// Note: Here we assume that F is known!

	Matrix3f F;
	F << -0.936791,      2.24,  -26.1548,
	1.03284,   1.77101,  -13.5179,
	26.3407,   10.5485,         1;

	// For the Original Input Points, Compute the Epipolar Lines

	vector<vec4> elines;

	elines.clear();
	for(auto& op: tr.originpoints){
		vec2 n = op;
		n.x = 0.5f*( n.x * ( 6.75 / 12.0 ) + 1.0f);
		n.y = 0.5f*(-n.y * ( 6.75 / 12.0 ) + ( 6.75 / 12.0 ) ) ;
		vec3 neline = unp::eline(F, n);
		elines.emplace_back(neline.x, neline.y, neline.z, 0);

		//
		cout<<"ELINE: "<<neline.x<<" "<<neline.y<<" "<<neline.z<<endl;

	}
	lbuf.fill(elines);

	// WARP THE POINTS TO THE NEAREST POINT ON THE EPIPOLAR LINE

	for(size_t i = 0; i < tr.points.size(); i++){

		vec2 n = tr.points[i];
		vec4 e = elines[i];

		// Map to [0, 1] range
		n.x = 0.5f*( n.x * ( 6.75 / 12.0 ) + 1.0f);
		n.y = 0.5f*(-n.y * ( 6.75 / 12.0 ) + ( 6.75 / 12.0 ) ) ;

		// Nearest Point on Epipolar Line

		vec2 tn;
		tn.x = (e.y*( e.y*n.x - e.x*n.y) - e.x*e.z)/(e.x*e.x + e.y*e.y);
		tn.y = (e.x*(-e.y*n.x + e.x*n.y) - e.y*e.z)/(e.x*e.x + e.y*e.y);
		n = tn;
		
		cout<<"DOT "<<dot(vec3(n.x, n.y, 1), vec3(e.x, e.y, e.z))<<endl;

		// Map Back

		n.x = ( 2.0f * n.x - 1.0f ) / ( 6.75 / 12.0 );
		n.y = - ( 2.0f * n.y - ( 6.75 / 12.0 ) ) / ( 6.75 / 12.0 );

	//	n *= vec2(1200);
	//	n.y = -n.y/(675.0f*0.5f)-1.0f;
	//	n.x = (n.x/(1200.0f*0.5f)-1.0f)*(12.0/6.75);

		tr.points[i] = n;

	}
	tri::pointbuf->fill(tr.points);



	Model lmesh({"in_Position"});
	lmesh.bind<vec4>("in_Position", &lbuf);
	lmesh.SIZE = elines.size();

	Shader epipolarline({"shader/epipolarline.vs", "shader/epipolarline.gs", "shader/epipolarline.fs"}, {"in_Position"});











	// Convenience Lambdas

	auto doreset = [&](){

		reset.use();
		reset.uniform("NTriangles", 13*tr.NT);
		reset.uniform("NPoints", tr.NP);

		if((13*tr.NT) > tr.NP) reset.dispatch(1 + (13*tr.NT)/1024);
		else reset.dispatch(1 + tr.NP/1024);

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

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

		/*
		reset.use();
		reset.uniform("NTriangles", 13*tr.NT);
		reset.uniform("NPoints", tr.NP);

		if((13*tr.NT) > tr.NP) reset.dispatch(1 + (13*tr.NT)/1024);
		else reset.dispatch(1 + tr.NP/1024);

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		*/

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);		//Load Texture
		triangleshader.uniform("mode", 2);
		triangleshader.uniform("KTriangles", tr.NT);
		triangleshader.uniform("RATIO", tri::RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, tr.NT);

//		point.use();
//		point.uniform("RATIO", tri::RATIO);
//		pointmesh.render(GL_POINTS);

		linestrip.use();
		linestrip.uniform("RATIO", tri::RATIO);
		linestripinstance.render(GL_LINE_STRIP, tr.NT);

		epipolarline.use();
		epipolarline.uniform("color", vec3(1,0,1));
		lmesh.render(GL_POINTS);

	};

	// Main Functions
	doreset();

	Tiny::view.pipeline = [&](){

		Tiny::view.target(color::black);				//Target Main Screen

		draw();

	};

	Tiny::loop([&](){

		if(paused) return;

		// Compute Cost and Gradients, Shift Points

		doreset();
		doenergy();
		doshift();

		// Retrieve Data from Compute Shader

		tri::tenergybuf->retrieve((13*tr.NT), tri::terr);
		tri::penergybuf->retrieve(tr.NP, tri::perr);
		tri::tcolnumbuf->retrieve((13*tr.NT), tri::cn);
		tri::pointbuf->retrieve(tr.points);

		cout<<tri::perr[10]<<endl;
//		cout<<tri::terr[0]<<endl;

	//	if(donext){
		if( tri::geterr(&tr) < 1E-6 ){

			donext = false;
			cout<<"RETRIANGULATE"<<endl;

		//	paused = true;

			if(!importlist.empty()){

				tr.read("../../output/"+outfolder+"/"+to_string(importlist.back())+".tri");
				tri::upload(&tr);
				importlist.pop_back();


				elines.clear();
				for(auto& op: tr.originpoints){
					vec2 nop = op;
					nop.x = 0.5f*(nop.x / (12.0/6.75) + 1.0f)*1200;
					nop.y = 0.5f*(-nop.y + 1.0f)*675;
					nop /= vec2(1200);
					vec3 neline = unp::eline(F, nop);
					elines.emplace_back(neline.x, neline.y, neline.z, 0);
				}
				lbuf.fill(elines);
				lmesh.SIZE = elines.size();


				doreset();
				doenergy();

				draw();

			}

			else{

				// Output the Triangulation (With Energy)
				Tiny::event.quit = true;
				tr.write("../../output/"+outfolder+"/out.tri");
				io::writeenergy(&tr, "../../output/"+outfolder+"/energy.txt");
				return;

			}

		}

	});


	tri::quit();
	Tiny::quit();

	return 0;
}
