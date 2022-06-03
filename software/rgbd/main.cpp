#include <TinyEngine/TinyEngine>
#include <TinyEngine/parse>
#include <TinyEngine/image>
#include <TinyEngine/color>
#include <TinyEngine/camera>

#include <tpose/tpose>
#include <tpose/triangulation>
#include <tpose/io>

#include <string>
#include <iostream>

#include <Eigen/Dense>

#include <piolib/dataset/loader>
#include <piolib/point/project>
#include <piolib/point/sample>
#include <piolib/point/io>
#include <piolib/point/pca>
#include <piolib/pose/io>

using namespace glm;
using namespace std;

int main( int argc, char* args[] ) {

	// Commandline Argument Parsing

	parse::get(argc, args);

	if(!parse::option.contains("d")){
		cout<<"Please specify an input depth image with -d."<<endl;
		exit(0);
	}

	if(!parse::option.contains("rgb")){
		cout<<"Please specify an input rgb image with -rgb."<<endl;
		exit(0);
	}

	if(!parse::option.contains("t")){
		cout<<"Please specify a first input triangulation with -t."<<endl;
		exit(0);
	}

	// Load Data

	SDL_Surface* IMG = IMG_Load(parse::option["rgb"].c_str());
	if(IMG == NULL){
		cout<<"Failed to load depth image."<<endl;
		exit(0);
	}

	string ta = parse::option["t"];

	// Load Frames

	cout<<"Setting up Frame..."<<endl;

	rs2::frameset frames;

	pio::loader::NBUNDLE = 2;
	pio::loader::load_infra = false;
	pio::loader::load_accel = false;
	pio::loader::load_gyros = false;
	pio::loader::set_aligned( RS2_STREAM_COLOR );
	pio::loader::open();

	if(!pio::loader::getframes(frames, {parse::option["d"], parse::option["rgb"]}))
		exit(0);

	cout<<"Retrieved Frames"<<endl;













	Tiny::view.pointSize = 2.0f;
	Tiny::view.vsync = false;
	Tiny::view.antialias = 0;

	Tiny::window("Energy Based Image Triangulation, Nicholas Mcdonald 2022", IMG->w, IMG->h);
	tpose::RATIO = (float)IMG->w/(float)IMG->h;

	const mat3 T = transpose(mat3(
		1.0f / tpose::RATIO * 0.5f * 960.0f, 0, 0.5f * 960.0f,
		0.0f, 1.0f * 0.5f * 540.0f, 0.5f * 540.0f,
		0.0f, 0.0f, 1.0f
	));
	const mat3 TB = inverse(T);

	cam::far = 100.0f;                          //Projection Matrix Far-Clip
	cam::near = 0.1f;
	cam::moverate = 0.01f;
	cam::turnrate = 0.15f;
	cam::init();

	bool showlines = false;
	bool showpoints = false;

	Tiny::view.interface = [](){};
	Tiny::event.handler = [&](){

		cam::handler();

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_n)
			showlines = !showlines;

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_p)
			showpoints = !showpoints;

	};

	glDisable(GL_CULL_FACE);

	tpose::init();

	Texture tex(IMG);		//Load Texture with Image

	// 2D Triangles and Line Strips

	Triangle triangle;
	Instance triangleinstance(&triangle);

	Shader triangleshader({"shader/triangle.vs", "shader/triangle.fs"}, {"in_Position"}, {"points", "index", "colacc"});
	triangleshader.bind<vec2>("points", tpose::pointbuf);
	triangleshader.bind<ivec4>("index", tpose::trianglebuf);
	triangleshader.bind<ivec4>("colacc", tpose::tcolaccbuf);

	TLineStrip tlinestrip;
	Instance linestripinstance(&tlinestrip);

	Shader linestrip({"shader/linestrip.vs", "shader/linestrip.fs"}, {"in_Position"}, {"points", "index"});
	linestrip.bind<vec2>("points", tpose::pointbuf);
	linestrip.bind<ivec4>("index", tpose::trianglebuf);

	// 3D Triangles

	Buffer npointbuf, npixelbuf, ntrianglebuf, ncolbuf;

	Shader triangle3D({"shader/triangle3D.vs", "shader/triangle3D.fs"}, {"in_Position"}, {"points3D", "otherindex", "othercol", "points2D"});
	triangle3D.bind<vec4>("points3D", &npointbuf);
	triangle3D.bind<ivec4>("otherindex", &ntrianglebuf);
	triangle3D.bind<vec4>("othercol", &ncolbuf);
	triangle3D.bind<vec2>("points2D", &npixelbuf);

	// 3D Particles

	Shader particle({"shader/particle.vs", "shader/particle.fs"}, {"in_Position", "in_Color", "in_Normal"});

	Buffer pointbuf, colorbuf, normalbuf;

	Model particles({"in_Position", "in_Color", "in_Normal"});
  particles.bind<glm::vec4>("in_Position", &pointbuf);
	particles.bind<glm::vec4>("in_Color", &colorbuf);
	particles.bind<glm::vec4>("in_Normal", &normalbuf);








	// Unproject Raw RGB-D Data

	vector<glm::vec4> pointset;
	vector<glm::vec4> colorset;
	vector<glm::vec4> normalset;
	vector<glm::vec2> pixelset;

	rs2::frame colorframe = frames.first(RS2_STREAM_COLOR);
	uint8_t* dat = (uint8_t*)colorframe.get_data();           //Raw Byte Data
	size_t size = colorframe.get_data_size();                 //Number in Bytes!

	const function<glm::vec4(int)> colortex = [&](const int i){
		float r = dat[4*i+0]/255.0f;
		float g = dat[4*i+1]/255.0f;
		float b = dat[4*i+2]/255.0f;
		return glm::vec4(r,g,b,1);
	};

	rs2::pointcloud pc;
	rs2::points points;

	points = pc.calculate(frames.first(RS2_STREAM_DEPTH));
	auto vertices = points.get_vertices();

	for(size_t i = 0; i < points.size(); i++){

		if(!vertices[i].z) continue;

		glm::vec3 pos = glm::vec3(-vertices[i].x, -vertices[i].y, vertices[i].z);

		pointset.push_back(glm::vec4(pos, 1.0f));
		colorset.push_back(colortex(i));
		normalset.push_back(vec4(1));

	}

	pio::sample::donormalmean = false;
	pio::point::pcanode* root = pio::sample::pcatree<pio::point::pcaMaxEW>(pointset, colorset, normalset, 1024*32);
	delete root;

	for(auto& p: pointset)
		pixelset.push_back(pio::point::project(p, pio::loader::intrColor));

	pointbuf.fill(pointset);
	colorbuf.fill(colorset);
	normalbuf.fill(normalset);
  particles.SIZE = pointset.size();






	// Load Triangulation

	tpose::triangulation trA;
	while(tpose::io::read(&trA, ta));	// Get Finest Triangulation
	tpose::upload(&trA);

	vector<ivec4> ntriangles;
	vector<vec4> nvertices;
	vector<float> nring;
	vector<vec2> npixels;
	vector<vec4> ncolors;

	for(size_t i = 0; i < trA.points.size(); i++){
		nvertices.push_back(vec4(0));
		nring.push_back(0);
	}

	for(size_t t = 0; t < trA.triangles.size(); t++){

		// Add the Triangle

		// Compute the number of Points inside this Triangle

		// First: Compute Min / Max Coordinates for Iteration

		glm::vec2 min = trA.points[trA.triangles[t][0]];
		glm::vec2 max = trA.points[trA.triangles[t][0]];

		min = glm::min(min, trA.points[trA.triangles[t][1]]);
		max = glm::max(max, trA.points[trA.triangles[t][1]]);

		min = glm::min(min, trA.points[trA.triangles[t][2]]);
		max = glm::max(max, trA.points[trA.triangles[t][2]]);

		min = T*vec3(min, 1.0f);
		max = T*vec3(max, 1.0f);

		set<size_t> indices;

		for(size_t i = 0; i < pixelset.size(); i++){

			if(pixelset[i].x < min.x) continue;
			if(pixelset[i].x > max.x) continue;
			if(pixelset[i].y < min.y) continue;
			if(pixelset[i].y > max.y) continue;

			if(!tpose::intriangle(TB*vec3(pixelset[i], 1.0f), trA.triangles[t], trA.points)) continue;

			indices.insert(i);

		}

		if(indices.size() > 0){


		//	if(indices.size() < pio::point::PCAMINSIZE)
		//		continue;

			// Perform PCA Fit

			vec3 pcamean = vec3(0);
			vec3 pcanormal = vec3(0);

		//	pio::point::pca PCA(pointset, indices, mat4(1.0f));
		//	pcamean = vec3(PCA.mean(0), PCA.mean(1), PCA.mean(2));
		//	pcanormal = vec3(PCA.normal(0), PCA.normal(1), PCA.normal(2));



			for(auto& i: indices){
				pcamean += vec3(pointset[i]);
				pcanormal += vec3(normalset[i]);
			}
			pcamean /= (float)indices.size();
			pcanormal /= (float)indices.size();

			pcanormal = normalize(pcanormal);



			// Unproject the Vertices

			// Basically take the 2D Position, compute a ray somehow, find the intersection with the plane in 3-Space

			// 1.: Un-Distort point in 2D
			// 2.: Ray-Cast using Pinhole Camera
			// 3.: Intersect Plane!

			ivec4 tr = trA.triangles[t];	// This Triangle (Triangulation)
			vec2 p0 = trA.points[tr[0]];	// Point 0 in 2D
			vec2 p1 = trA.points[tr[1]];	// Point 1 in 2D
			vec2 p2 = trA.points[tr[2]];	// Point 2 in 2D

		//	npixels.push_back(p0);
		//	npixels.push_back(p1);
		//	npixels.push_back(p2);

			vec4 pp0 = pio::point::unproject( T*vec3(p0, 1.0f), pio::loader::intrColor, pcamean, pcanormal);
			vec4 pp1 = pio::point::unproject( T*vec3(p1, 1.0f), pio::loader::intrColor, pcamean, pcanormal);
			vec4 pp2 = pio::point::unproject( T*vec3(p2, 1.0f), pio::loader::intrColor, pcamean, pcanormal);

			nvertices[trA.triangles[t][0]] += pp0;
			nvertices[trA.triangles[t][1]] += pp1;
			nvertices[trA.triangles[t][2]] += pp2;
			nring[trA.triangles[t][0]] += 1.0f;
			nring[trA.triangles[t][1]] += 1.0f;
			nring[trA.triangles[t][2]] += 1.0f;

		}

		ntriangles.push_back(trA.triangles[t]);
		ncolors.push_back((vec4)tpose::col[t]/255.0f);
		ncolors.back().w = 1.0f;

	}

	for(size_t i = 0; i < trA.points.size(); i++)
		nvertices[i] /= nring[i];

	npointbuf.fill(nvertices);
	ntrianglebuf.fill(ntriangles);
	ncolbuf.fill(ncolors);
	npixelbuf.fill(trA.points);

	// Color Accumulation Buffers


	// Main Functions

	Tiny::view.pipeline = [&](){

		Tiny::view.target(color::black);				//Target Main Screen

		if(showlines){

			triangleshader.use();
			triangleshader.texture("imageTexture", tex);		//Load Texture
			triangleshader.uniform("KTriangles", trA.NT);
			triangleshader.uniform("RATIO", tpose::RATIO);
			triangleinstance.render(GL_TRIANGLE_STRIP, trA.NT);

			linestrip.use();
			linestrip.uniform("KTriangles", trA.NT);
			linestrip.uniform("RATIO", tpose::RATIO);
			linestripinstance.render(GL_LINE_STRIP, trA.NT);

		}

		else {

			if(showpoints){

				particle.use();
				particle.uniform("vp", cam::vp);
				particles.render(GL_POINTS);

			}

			else {

				triangle3D.use();
				triangle3D.texture("imageTexture", tex);		//Load Texture
				triangle3D.uniform("model", glm::mat4(1.0f));
				triangle3D.uniform("vp", cam::vp);
				triangle3D.uniform("RATIO", tpose::RATIO);
				triangleinstance.render(GL_TRIANGLE_STRIP, trA.NT);

			}

		}

	};

	Tiny::loop([&](){});



	tpose::quit();
	Tiny::quit();
	pio::loader::close();
	delete IMG;

	return 0;

}
