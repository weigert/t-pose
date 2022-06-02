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
	vector<glm::vec4> normalset;

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
	//	point2D.push_back(glm::vec2( i % sd->w, sd->h - i / sd->w ));
		//point2D.push_back(glm::vec2( i / sd->h, i % sd->h ));
		point3D.push_back(glm::vec4(pos, 1.0f));
		colorset.push_back(colortex(i));
		normalset.push_back(vec4(1));

	}

	pio::sample::donormalmean = false;

	pio::point::pcanode* root = pio::sample::pcatree<pio::point::pcaMaxEW>(point3D, colorset, normalset, 1024*8);
	delete root;

	// Instead of adding the point2D directly, we should project the 3D points to pixel coordinates.
	// Then we can do a downsampling in 3D.

	function<vec2(vec4)> project = [&](vec4 p){

		float x = -p[0] / p[2], y = p[1] / p[2];

		float r2 = x * x + y * y;
		float f = 1 + pio::loader::intrColor.coeffs[0] * r2 + pio::loader::intrColor.coeffs[1] * r2 * r2 + pio::loader::intrColor.coeffs[4] * r2 * r2 * r2;
		x *= f;
		y *= f;
		float dx = x + 2 * pio::loader::intrColor.coeffs[2] * x * y + pio::loader::intrColor.coeffs[3] * (r2 + 2 * x * x);
		float dy = y + 2 * pio::loader::intrColor.coeffs[3] * x * y + pio::loader::intrColor.coeffs[2] * (r2 + 2 * y * y);
		x = dx;
		y = dy;

		x = x * pio::loader::intrColor.fx + pio::loader::intrColor.ppx;
    y = y * pio::loader::intrColor.fy + pio::loader::intrColor.ppy;

		return vec2(x, y);

	};


	for(auto& p: point3D){
		point2D.push_back(project(p));
	}

//	cout<<point2D[500][0]<<" "<<point2D[500][1]<<endl;
//	vec2 p2d = project(point3D[500]);
//	cout<<p2d[0]<<" "<<p2d[1]<<endl;


	cout<<"Found Points: "<<point3D.size()<<endl;
	cout<<"Found Points: "<<point2D.size()<<endl;

	Tiny::view.pointSize = 2.0f;
	Tiny::view.vsync = false;
	Tiny::view.antialias = 0;

	Tiny::window("Energy Based Image Triangulation, Nicholas Mcdonald 2022", IMG->w, IMG->h);
	tri::RATIO = (float)IMG->w/(float)IMG->h;

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

		//if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_p)
		//	paused = !paused;

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_n)
			showlines = !showlines;

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_p)
			showpoints = !showpoints;

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
	Buffer point2Dbuf;
	Buffer triangle3Dbuf;
	Buffer col3Dbuf;

	Shader triangle3D({"shader/triangle3D.vs", "shader/triangle3D.fs"}, {"in_Position"}, {"points3D", "otherindex", "othercol", "points2D"});
	triangle3D.bind<vec4>("points3D", &point3Dbuf);
	triangle3D.bind<ivec4>("otherindex", &triangle3Dbuf);
	triangle3D.bind<vec4>("othercol", &col3Dbuf);
	triangle3D.bind<vec2>("points2D", &point2Dbuf);

	Shader linestrip({"shader/linestrip.vs", "shader/linestrip.fs"}, {"in_Position"}, {"points", "index"});
	linestrip.bind<vec2>("points", tri::pointbuf);
	linestrip.bind<ivec4>("index", tri::trianglebuf);

	Shader particle({"shader/particle.vs", "shader/particle.fs"}, {"in_Position", "in_Color", "in_Normal"});

	Buffer positionbuf, colorbuf, normalbuf;

	Model particles({"in_Position", "in_Color", "in_Normal"});
  particles.bind<glm::vec4>("in_Position", &positionbuf);
	particles.bind<glm::vec4>("in_Color", &colorbuf);
	particles.bind<glm::vec4>("in_Normal", &normalbuf);
  particles.SIZE = point3D.size();

	positionbuf.fill(point3D);
	colorbuf.fill(colorset);
	normalbuf.fill(normalset);

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
	vector<vec2> tpoints2D;
	vector<vec4> tcolors;

	int NP = 0;

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

		min.x = (min.x / tri::RATIO + 1.0f) * 0.5 * sd->w;
		max.x = (max.x / tri::RATIO + 1.0f) * 0.5 * sd->w;

		min.y = (min.y + 1.0f) * 0.5 * sd->h;
		max.y = (max.y + 1.0f) * 0.5 * sd->h;

	//	cout<<min.x<<" "<<min.y<<endl;
	//	cout<<max.x<<" "<<max.y<<endl;

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

		if(indices.size() == 0)
			continue;

		//if(indices.size() < pio::point::PCAMINSIZE)
		//	continue;

		NP += indices.size();

		// Perform PCA Fit

		vec3 pcamean = vec3(0);
		vec3 pcanormal = vec3(0);

//		pio::point::pca PCA(point3D, indices, mat4(1.0f));
//		pcamean = vec3(PCA.mean(0), PCA.mean(1), PCA.mean(2));
//		pcanormal = vec3(PCA.normal(0), PCA.normal(1), PCA.normal(2));

		for(auto& i: indices){
			pcamean += vec3(point3D[i]);
			pcanormal += vec3(normalset[i]);
		}
		pcamean /= (float)indices.size();
		pcanormal /= (float)indices.size();

		// Unproject the Vertices

		// Basically take the 2D Position, compute a ray somehow, find the intersection with the plane in 3-Space

		ivec4 tr = trA.triangles[t];	// This Triangle (Triangulation)
		vec2 p0 = trA.points[tr[0]];	// Point 0 in 2D
		vec2 p1 = trA.points[tr[1]];	// Point 1 in 2D
		vec2 p2 = trA.points[tr[2]];	// Point 2 in 2D

		// 1.: Un-Distort point in 2D
		// 2.: Ray-Cast using Pinhole Camera
		// 3.: Intersect Plane!

		std::function<vec4(vec2)> unproject = [&](vec2 p){

			vec2 pixel = ( p / vec2( tri::RATIO, 1.0f ) + 1.0f ) * 0.5f * vec2( sd->w, sd->h );

			float x = (pixel[0] - pio::loader::intrColor.ppx) / pio::loader::intrColor.fx;
			float y = (pixel[1] - pio::loader::intrColor.ppy) / pio::loader::intrColor.fy;

			float xo = x;
			float yo = y;

			for (int i = 0; i < 10; i++){

					float r2 = x * x + y * y;
					float icdist = (float)1 / (float)(1 + ((pio::loader::intrColor.coeffs[4] * r2 + pio::loader::intrColor.coeffs[1]) * r2 + pio::loader::intrColor.coeffs[0]) * r2);
					float xq = x / icdist;
					float yq = y / icdist;
					float delta_x = 2 * pio::loader::intrColor.coeffs[2] * xq * yq + pio::loader::intrColor.coeffs[3] * (r2 + 2 * xq * xq);
					float delta_y = 2 * pio::loader::intrColor.coeffs[3] * xq * yq + pio::loader::intrColor.coeffs[2] * (r2 + 2 * yq * yq);
					x = (xo - delta_x) * icdist;
					y = (yo - delta_y) * icdist;

			}

			//Solve for depth s.t. this point satisfied the plane equation!

						//; // this is basically the sliding parameter!

			float depth = abs( dot( pcamean, pcanormal ) / dot( vec3(-x, y, 1), pcanormal ) );

			vec4 point;
			point[0] = -depth * x;
			point[1] = depth * y;
			point[2] = depth;
			point[3] = 1.0f;

			return point;

		};

		vec4 pp0 = unproject(p0);
		vec4 pp1 = unproject(p1);
		vec4 pp2 = unproject(p2);





		// Compute the Average Distance to the plane defined by this guy!

		vec3 nn = normalize(glm::cross(vec3(pp1 - pp0), vec3(pp2 - pp0)));
		vec3 mean = vec3(pp0 + pp1 + pp2)/3.0f;

		/*
		float d = 0.0f;
		for(auto& i: indices)
			d += abs(dot(vec3(point3D[i]) - mean, nn));
		d /= (float)indices.size();

		cout<<d<<endl;

		if(d > 0.05) continue;
		*/


	//	if(dot(nn, normalize(mean)) < 0.1)
	//		continue;

		// Check inconsistency?


		// Add the Triangle

		tvertices.push_back(pp0);
		tvertices.push_back(pp1);
		tvertices.push_back(pp2);

		tpoints2D.push_back(p0);
		tpoints2D.push_back(p1);
		tpoints2D.push_back(p2);

		int nt = triangles.size();
		triangles.emplace_back(3*nt + 0, 3*nt + 1, 3*nt + 2, 0);
		tcolors.push_back((vec4)tri::col[t]/255.0f);
		tcolors.back().w = 1.0f;

	}

	cout<<point3D.size()<<endl;
	cout<<point2D.size()<<endl;
	cout<<NP<<endl;

	point3Dbuf.fill(tvertices);
	triangle3Dbuf.fill(triangles);
	col3Dbuf.fill(tcolors);
	point2Dbuf.fill(tpoints2D);

	// Color Accumulation Buffers

	triangleshader.bind<ivec4>("colacc", tri::tcolaccbuf);

	auto draw = [&](){

		/*


	//	point.use();
	//	point.uniform("RATIO", tri::RATIO);
	//	pointmesh.render(GL_POINTS);


		*/


		if(showlines){

			triangleshader.use();
			triangleshader.texture("imageTexture", tex);		//Load Texture
			triangleshader.uniform("mode", 2);
			triangleshader.uniform("K", trA.NT);
			triangleshader.uniform("RATIO", tri::RATIO);
			triangleinstance.render(GL_TRIANGLE_STRIP, trA.NT);

			linestrip.use();
			linestrip.uniform("RATIO", tri::RATIO);
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
				triangle3D.uniform("RATIO", tri::RATIO);
				triangleinstance.render(GL_TRIANGLE_STRIP, trA.NT);

			}

		}

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
