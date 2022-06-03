#include <TinyEngine/TinyEngine>
#include <TinyEngine/image>
#include <TinyEngine/color>
#include <TinyEngine/camera>

#include <tpose/tpose>
#include <tpose/io>
#include <tpose/multiview>

using namespace std;
using namespace glm;
using namespace Eigen;

int main( int argc, char* args[] ) {

	// Load Raw Pointmatches

	vector<vec2> A, B;
	if(!tpose::io::readmatches("data.txt", A, B))
		exit(0);

	cout << "Loaded " << A.size() << " point matches" << endl;

	// Normalize points to Image Width

	for(auto& a: A)
		a /= vec2(1200);

	for(auto& b: B)
		b /= vec2(1200);

	// Unproject the 2D Matches to 3D Points

	// Fundamental Matrix

	Matrix3f F = tpose::mview::F_LMEDS(A, B);
	cout<<"FUNDAMENTAL MATRIX: "<<F<<endl;

	Matrix3f K = tpose::mview::Camera();

	// Triangulate all Points

	vector<vec4> points3d = tpose::mview::triangulate(F, K, A, B);

	vector<vec3> lA, lB; //Epipolar Lines

	for(size_t i = 0; i < A.size(); i++)
		lA.push_back(tpose::mview::eline(B[i], F));

	for(size_t i = 0; i < B.size(); i++)
		lB.push_back(tpose::mview::eline(F, A[i]));

	Tiny::view.pointSize = 5.0f;
	Tiny::window("Point-Match Reconstruction, Nicholas Mcdonald 2022", 1200, 675);
	Tiny::view.interface = [&](){};

	// Image Rendering

	Shader image({"shader/image.vs", "shader/image.fs"}, {"in_Quad", "in_Tex"});

	Texture texA(image::load("../../resource/imageA.png"));		//Load Texture with Image
	Texture texB(image::load("../../resource/imageB.png"));		//Load Texture with Image
	Square2D flat;														//Create Primitive Model

	// Feature Point / Line Rendering

	Shader point({"shader/point.vs", "shader/point.fs"}, {"in_Position"});
	Shader point3d({"shader/point3d.vs", "shader/point3d.fs"}, {"in_Position"});

	cam::far = 100.0f;                          //Projection Matrix Far-Clip
	cam::near = 0.001f;
	cam::moverate = 0.05f;
	cam::FOV = 0.5;
	cam::init();
	cam::look = vec3(0,0,7);
	cam::update();

	Buffer pbufA(A);
	Buffer pbufB(B);

	Model pmeshA({"in_Position"});
	pmeshA.bind<vec2>("in_Position", &pbufA);
	pmeshA.SIZE = A.size();

	Model pmeshB({"in_Position"});
	pmeshB.bind<vec2>("in_Position", &pbufB);
	pmeshB.SIZE = B.size();

	Buffer pbuf3D(points3d);
	Model pmesh3D({"in_Position"});
	pmesh3D.bind<vec4>("in_Position", &pbuf3D);
	pmesh3D.SIZE = points3d.size();

	vector<vec2> linevec;
	for(size_t n = 0; n < A.size(); n++){
		linevec.push_back(A[n]);
		linevec.push_back(B[n]);
	}
	Buffer linebuf(linevec);

	Model linemesh({"in_Position"});
	linemesh.bind<vec2>("in_Position", &linebuf);
	linemesh.SIZE = linevec.size();

	// Epipolar Line Rendering

	Shader epipolarline({"shader/epipolarline.vs", "shader/epipolarline.gs", "shader/epipolarline.fs"}, {"in_Position"});

	Buffer lbufA(lA);
	Buffer lbufB(lB);

	Model lmeshA({"in_Position"});
	lmeshA.bind<vec3>("in_Position", &lbufA);
	lmeshA.SIZE = lA.size();

	Model lmeshB({"in_Position"});
	lmeshB.bind<vec3>("in_Position", &lbufB);
	lmeshB.SIZE = lB.size();

	// The triangulation needs to be

	bool flip = true;
	bool view3d = false;
	Tiny::event.handler = [&](){

		cam::handler();

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_SPACE)
			flip = !flip;
		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_m)
			view3d = !view3d;

	};

	Tiny::view.pipeline = [&](){

		Tiny::view.target(color::black);				//Target Main Screen

		if(!view3d){

			image.use();														//Use Effect Shader
			image.texture("imageTextureA", texA);		//Load Texture
			image.texture("imageTextureB", texB);		//Load Texture
			image.uniform("flip", flip);
			image.uniform("model", flat.model);		//Add Model Matrix
			flat.render();


			if(flip) {

				point.use();
				point.uniform("color", vec3(1,0,0));
				pmeshA.render(GL_POINTS);

				epipolarline.use();
				epipolarline.uniform("color", vec3(1,0,0));
				lmeshA.render(GL_POINTS);

			}

			else {

				point.use();
				point.uniform("color", vec3(1,0,1));
				pmeshB.render(GL_POINTS);

				epipolarline.use();
				epipolarline.uniform("color", vec3(1,0,1));
				lmeshB.render(GL_POINTS);

			}

			point.use();
			point.uniform("color", vec3(1,1,1));
			linemesh.render(GL_LINES);

		}

		else{

			point3d.use();
			point3d.uniform("model", rotate(mat4(1.0f), 3.14159265f, vec3(0,0,1)));
			point3d.uniform("vp", cam::vp);
			pmesh3D.render(GL_POINTS);

		}

	};

	Tiny::loop([&](){});
	Tiny::quit();

	return 0;
}
