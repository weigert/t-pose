// RGBD Mesh

/*
    We need to do a mix of things here...
    Load Triangulation
    Load Depth Image
    Unproject Points
    etc. etc.
*/


#include <TinyEngine/TinyEngine>
#include <TinyEngine/parse>
#include <TinyEngine/image>
#include <TinyEngine/color>
#include <TinyEngine/camera>

#include "../../../source/triangulate.h"

#include <string>
#include <iostream>

#include <Eigen/Dense>

#include <piolib/dataset/loader>
#include <piolib/point/sample>
#include <piolib/point/io>
#include <piolib/point/pca>
#include <piolib/pose/io>

using namespace glm;
using namespace std;

int main( int argc, char* args[] ) {

	parse::get(argc, args);

	string outfolder;

	SDL_Surface* IMG = NULL;
	if(!parse::option.contains("d")){
		cout<<"Please specify an input depth image with -d."<<endl;
		exit(0);
	}
	else IMG = IMG_Load(parse::option["d"].c_str());
	if(IMG == NULL){
		cout<<"Failed to load depth image."<<endl;
		exit(0);
	}

	string ta;
	if(!parse::option.contains("t")){
		cout<<"Please specify a first input triangulation with -t."<<endl;
		exit(0);
	}
	else{
		ta = parse::option["t"];
	}

	// Compute the Pointset!

	// Store it in a map???

	/*
		Somehow get the data into the frame...
		Then unproject the points...
		Fit each triangle, visualize it like that...
		Somehow find the point of the vertices in 3-space
	*/

	// Basically I need to get the data into the frame...
	// Then I need to process the frame???
	//

	cout<<"Setting up Frame..."<<endl;

	rs2::frameset frames;

	// Stream setup: Note they are pre-aligned!!

	pio::loader::color_stream = pio::loader::color_sensor.add_video_stream({RS2_STREAM_COLOR, 1, 1, pio::loader::CW, pio::loader::CH, 100, pio::loader::CB, RS2_FORMAT_BGRA8, pio::loader::intrColor });
	pio::loader::depth_stream = pio::loader::depth_sensor.add_video_stream({RS2_STREAM_DEPTH, 0, 0, pio::loader::CW, pio::loader::CH, 100, pio::loader::DB, RS2_FORMAT_Z16, pio::loader::intrColor });

	pio::loader::depth_sensor.add_read_only_option(RS2_OPTION_DEPTH_UNITS, 0.00025f);

	pio::loader::color_stream.register_extrinsics_to(pio::loader::depth_stream, pio::loader::extrIdentity);
  pio::loader::depth_stream.register_extrinsics_to(pio::loader::color_stream, pio::loader::extrIdentity);

	pio::loader::depth_sensor.open(pio::loader::depth_stream);
  pio::loader::color_sensor.open(pio::loader::color_stream);

	pio::loader::bundler.start(pio::loader::q);
	pio::loader::depth_sensor.start([&](rs2::frame f){
		pio::loader::bundler.invoke(f);
	});
	pio::loader::color_sensor.start([&](rs2::frame f){
		pio::loader::bundler.invoke(f);
	});

	// Create Synthetic frames

	pio::loader::synthframe cframe(960, 540, 4);
	pio::loader::synthframe dframe(960, 540, 2);

	cframe.dat = new uint8_t[960*540*4];
	dframe.dat = new uint8_t[940*540*2];

	// Load the actual Data

	SDL_Surface* srgb = NULL;
	srgb = IMG_Load("rgb.png");
	if(srgb == NULL){
		cout<<"FAILED TO LOAD RGB IMAGE"<<endl;
		return 0;
	}

	SDL_Surface* sd = NULL;
	sd = IMG_Load("d.png");
	if(sd == NULL){
		cout<<"FAILED TO LOAD DEPTH IMAGE"<<endl;
		return 0;
	}

	// Iterate over the Images, fill the Frames Correctly!

	SDL_LockSurface(srgb);
	unsigned char* img_raw = (unsigned char*)srgb->pixels; //Raw Data
	for(int i = 0; i < srgb->w*srgb->h; i++){
		cframe.dat[4*i+0] = *(img_raw+4*i+0);
		cframe.dat[4*i+1] = *(img_raw+4*i+1);
		cframe.dat[4*i+2] = *(img_raw+4*i+2);
		cframe.dat[4*i+3] = *(img_raw+4*i+3);
	}
	SDL_UnlockSurface(srgb);

	SDL_LockSurface(sd);
	img_raw = (unsigned char*)sd->pixels; //Raw Data
	for(int i = 0; i < sd->w*sd->h; i++){
		dframe.dat[2*i+0] = *(img_raw+4*i+2);
		dframe.dat[2*i+1] = *(img_raw+4*i+1);
	}
	SDL_UnlockSurface(sd);

	// On-Frame the Sensors

	pio::loader::color_sensor.on_video_frame({ cframe.dat, [](void* dat){ delete[] (uint8_t*)dat; },
			960*540, 540, // Stride and Bytes-per-pixel
			0.0, RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, 0,
			pio::loader::color_stream
	});

	pio::loader::depth_sensor.on_video_frame({ dframe.dat, [](void* dat){ delete[] (uint8_t*)dat; },
			960*540, 540, // Stride and Bytes-per-pixel
			0.0, RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, 0,
			pio::loader::depth_stream
	});

	frames = pio::loader::q.wait_for_frame();

	cout<<"Retrieved Frames"<<endl;

	// Compute the Pointcloud

	vector<glm::vec4> point3D;
	vector<glm::vec2> point2D;
	vector<glm::vec4> colorset;

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

	//What is this frame?
	//cout<<t<<" "<<frames[t].first(RS2_STREAM_DEPTH).get_timestamp()<<endl;

	for(size_t i = 0; i < points.size(); i++){

		if(!vertices[i].z) continue;

		glm::vec3 pos = glm::vec3(-vertices[i].x, -vertices[i].y, vertices[i].z);

		//point2D.push_back(glm::vec2( i % sd->w, sd->h - i / sd->w ));
		point2D.push_back(glm::vec2( i % sd->w, sd->h - i / sd->w ));
		//point2D.push_back(glm::vec2( i / sd->h, i % sd->h ));
		point3D.push_back(glm::vec4(pos, 1.0f));
		colorset.push_back(colortex(i));

	}

	cout<<"Found Points: "<<point3D.size()<<endl;
	cout<<"Found Points: "<<point2D.size()<<endl;

	Tiny::view.pointSize = 2.0f;
	Tiny::view.vsync = false;
	Tiny::view.antialias = 0;

	Tiny::window("Energy Based Image Triangulation, Nicholas Mcdonald 2022", IMG->w, IMG->h);
	tri::RATIO = (float)IMG->w/(float)IMG->h;

	cam::far = 100.0f;                          //Projection Matrix Far-Clip
	cam::near = 0.1f;
	cam::moverate = 0.05f;
	cam::init();

	Tiny::view.interface = [](){};
	Tiny::event.handler = [&](){

		cam::handler();

		//if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_p)
		//	paused = !paused;

		//if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_n)
		//	showlines = !showlines;

	};

	Texture tex(IMG);		//Load Texture with Image
	Square2D flat;																						//Create Primitive Model

	Shader image({"shader/image.vs", "shader/image.fs"}, {"in_Quad", "in_Tex"});
	Shader point({"shader/point.vs", "shader/point.fs"}, {"in_Position"});

	glDisable(GL_CULL_FACE);

	tri::init();

	Triangle triangle;
	Instance triangleinstance(&triangle);

	TLineStrip tlinestrip;
	Instance linestripinstance(&tlinestrip);

	Shader triangleshader({"shader/triangle.vs", "shader/triangle.fs"}, {"in_Position"}, {"points", "index", "colacc"});
	triangleshader.bind<vec2>("points", tri::pointbuf);
	triangleshader.bind<ivec4>("index", tri::trianglebuf);

	Buffer point3Dbuf;
	Buffer triangle3Dbuf;
	Buffer col3Dbuf;

	Shader triangle3D({"shader/triangle3D.vs", "shader/triangle3D.fs"}, {"in_Position"}, {"points3D", "otherindex", "othercol"});
	triangle3D.bind<vec4>("points3D", &point3Dbuf);
	triangle3D.bind<ivec4>("otherindex", &triangle3Dbuf);
	triangle3D.bind<vec4>("othercol", &col3Dbuf);

	Shader linestrip({"shader/linestrip.vs", "shader/linestrip.fs"}, {"in_Position"}, {"points", "index"});
	linestrip.bind<vec2>("points", tri::pointbuf);
	linestrip.bind<ivec4>("index", tri::trianglebuf);

	Shader particle({"shader/particle.vs", "shader/particle.fs"}, {"in_Position", "in_Color"});

	Buffer positionbuf, colorbuf;

	Model particles({"in_Position", "in_Color"});
  particles.bind<glm::vec4>("in_Position", &positionbuf);
  particles.bind<glm::vec4>("in_Color", &colorbuf);
  particles.SIZE = point3D.size();

	positionbuf.fill(point3D);
	colorbuf.fill(colorset);

	Model pointmesh({"in_Position"});
	pointmesh.bind<vec2>("in_Position", tri::pointbuf);

	tri::triangulation trA;
	trA.read(ta, false);

	tri::upload(&trA);
	pointmesh.SIZE = trA.NP;


	// Compute the Triangle Positions!

	/*
			Iterate over all points in 2D, assign to triangle in 3D if it is in triangle in 2D
			Then we can fit the triangle but finding the actual vertex position would require
			fitting a ray to intersect the plane, which I don't really want to do.

			I can probably DOO IT ANYWAY, I'll just have to do the raycast math myself.
			Jesus I really wanted to avoid that.

			But I need to PCA-Fit the triangle anyway.

			I probably need to create a whole new triangle and vertex buffer.
			Then compute each vertex individually.
	*/














	vector<ivec4> triangles;
	vector<vec4> tvertices;
	vector<vec4> tcolors;

	int NP = 0;

	for(size_t t = 0; t < trA.triangles.size(); t++){

		// Add the Triangle

		triangles.emplace_back(3*t + 0, 3*t + 1, 3*t + 2, 0);
		tcolors.push_back((vec4)tri::col[t]/255.0f);
		tcolors.back().w = 1.0f;

		// Compute the number of Points inside this Triangle

		// First: Compute Min / Max Coordinates for Iteration

		glm::vec2 min = trA.points[trA.triangles[t][0]];
		glm::vec2 max = trA.points[trA.triangles[t][0]];

		min = glm::min(min, trA.points[trA.triangles[t][1]]);
		max = glm::max(max, trA.points[trA.triangles[t][1]]);

		min = glm::min(min, trA.points[trA.triangles[t][2]]);
		max = glm::max(max, trA.points[trA.triangles[t][2]]);

		min.x = (min.x / tri::RATIO + 1.0f) * 0.5 * sd->w;
		max.x = (max.x / tri::RATIO + 1.0f) * 0.5 * sd->w;

		min.y = (min.y + 1.0f) * 0.5 * sd->h;
		max.y = (max.y + 1.0f) * 0.5 * sd->h;

		cout<<min.x<<" "<<min.y<<endl;
		cout<<max.x<<" "<<max.y<<endl;

		// Extract all Point Indices

		set<size_t> indices;

		for(size_t i = 0; i < point2D.size(); i++){

			if(point2D[i].x < min.x) continue;
			if(point2D[i].x > max.x) continue;
			if(point2D[i].y < min.y) continue;
			if(point2D[i].y > max.y) continue;

			if(!intriangle(((point2D[i]/vec2(sd->w, sd->h)*2.0f-1.0f)*vec2(tri::RATIO, 1.0f)), trA.triangles[t], trA.points)) continue;

			indices.insert(i);

		}

		NP += indices.size();

		// Perform PCA Fit

		pio::point::pca PCA(point3D, indices, mat4(1.0f));



		// Unproject the Vertices

		vec3 p = vec3(PCA.mean(0), PCA.mean(1), PCA.mean(2));

		tvertices.emplace_back(p.x, p.y, p.z, 1);
		tvertices.emplace_back(p.x + 0.1*PCA.EV(0,1), p.y + 0.1*PCA.EV(1,1), p.z + 0.1*PCA.EV(2,1), 1);
		tvertices.emplace_back(p.x + 0.1*PCA.EV(0,2), p.y + 0.1*PCA.EV(1,2), p.z + 0.1*PCA.EV(2,2), 1);

	//	tvertices.emplace_back(p.x + PCA.EW(0)*PCA.EV(0,0), p.y + PCA.EW(0)*PCA.EV(1,0), p.z + PCA.EW(0)*PCA.EV(2,0), 1);
	//	tvertices.emplace_back(p.x + PCA.EW(1)*PCA.EV(0,1), p.y + PCA.EW(1)*PCA.EV(1,1), p.z + PCA.EW(1)*PCA.EV(2,1), 1);

//		tvertices.emplace_back(0,0,0,1);
	//	tvertices.emplace_back(p.x + 0.1, p.y + 0.0, p.z + 0.0, 1.0);
	//	tvertices.emplace_back(p.x + 0.0, p.y + 0.1, p.z + 0.0, 1.0);


		/*
		for(size_t i = 0; i < points.size(); i++){

			if(!vertices[i].z) continue;

			glm::vec3 pos = glm::vec3(-vertices[i].x, -vertices[i].y, vertices[i].z);

			point2D.push_back(glm::vec2( i / sd->w, i%sd->w ));
			point3D.push_back(glm::vec4(pos,1.0f));
			colorset.push_back(colortex(i));

		}
		*/



	}


	cout<<NP<<endl;
	cout<<point3D.size()<<endl;


	point3Dbuf.fill(tvertices);
	triangle3Dbuf.fill(triangles);
	col3Dbuf.fill(tcolors);



	// Color Accumulation Buffers

	triangleshader.bind<ivec4>("colacc", tri::tcolaccbuf);

	float s = 0.0f;

	auto draw = [&](){

		/*

		triangleshader.use();
		triangleshader.texture("imageTexture", tex);		//Load Texture
		triangleshader.uniform("mode", 2);
		triangleshader.uniform("K", trA.NT);
		triangleshader.uniform("RATIO", tri::RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, trA.NT);

	//	point.use();
	//	point.uniform("RATIO", tri::RATIO);
	//	pointmesh.render(GL_POINTS);


		*/

		triangle3D.use();
		triangle3D.texture("imageTexture", tex);		//Load Texture
		triangle3D.uniform("model", glm::mat4(1.0f));
		triangle3D.uniform("vp", cam::vp);
		triangle3D.uniform("RATIO", tri::RATIO);
		triangleinstance.render(GL_TRIANGLE_STRIP, trA.NT);

	//	linestrip.use();
	//	linestrip.uniform("RATIO", tri::RATIO);
	//	linestripinstance.render(GL_LINE_STRIP, trA.NT);


	//	particle.use();
	//	particle.uniform("vp", cam::vp);
	//	particles.render(GL_POINTS);

	};

	// Main Functions

	triangleshader.use();
	triangleshader.texture("imageTexture", tex);
	triangleshader.uniform("mode", 1);
	triangleshader.uniform("KTriangles", trA.NT);
	triangleshader.uniform("RATIO", tri::RATIO);
	triangleinstance.render(GL_TRIANGLE_STRIP, trA.NT);

	Tiny::view.pipeline = [&](){

		Tiny::view.target(color::black);				//Target Main Screen
		draw();

	};

	Tiny::loop([&](){});

	delete[] cframe.dat;
	delete[] dframe.dat;
	delete srgb;
	delete sd;

	tri::quit();
	Tiny::quit();

	return 0;

}
