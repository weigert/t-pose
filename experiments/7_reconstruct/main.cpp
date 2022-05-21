#include <TinyEngine/TinyEngine>
#include <TinyEngine/image>
#include <TinyEngine/color>
#include "camera.h"

#include "reconstruct.h"

#include <opencv2/opencv.hpp>
#include <opencv2/core/eigen.hpp>

int main( int argc, char* args[] ) {

	vector<vec2> A;
	vector<vec2> B;
	if(!read("data.txt", A, B)){
		exit(0);
	}

	cout<<"Loaded "<<A.size()<<" points."<<endl;

	// COMPUTE THE RECONSTRUCTION

	srand(time(NULL));
	//Matrix3f F = FundamentalRANSAC(A, B);

	// Try the OpenCV Method:




	vector<cv::Point2f> pointsA, pointsB;
	for(auto& a: A){
		pointsA.emplace_back(a.x, a.y);
	}

	for(auto& b: B){
		pointsB.emplace_back(b.x, b.y);
	}

	//cv::Mat cvF = cv::findFundamentalMat(pointsA, pointsB, cv::FM_RANSAC, 0.0025, 0.99);
	cv::Mat cvF = cv::findFundamentalMat(pointsA, pointsB, cv::FM_LMEDS, 0.0025, 0.99);

	Matrix3f F;
	cv2eigen(cvF, F);



//	Matrix3f F = Fundamental(A, B);

	cout<<"Fundamental Matrix F: "<<F<<endl;

	vector<vec4> points3d;

	for(size_t i = 0; i < A.size(); i++)
		points3d.push_back(triangulate(F, A[i], B[i], 0));

	for(size_t i = 0; i < A.size(); i++)
		points3d.push_back(triangulate(F, A[i], B[i], 1));

	for(size_t i = 0; i < A.size(); i++)
		points3d.push_back(triangulate(F, A[i], B[i], 2));

	for(size_t i = 0; i < A.size(); i++)
		points3d.push_back(triangulate(F, A[i], B[i], 3));

	// Question: How do I compute / visualize epipoles?

	vector<vec3> lA; //Epipolar Lines corresponding to points A
	vector<vec3> lB; //Epipolar Lines corresponding to points B

	for(size_t i = 0; i < A.size(); i++)
		lA.push_back(EpipolarLine(F.transpose(), B[i]));

	for(size_t i = 0; i < B.size(); i++)
		lB.push_back(EpipolarLine(F, A[i]));

	cout<<A.size()<<endl;

	//A.push_back(Epipole(F, true));
	//B.push_back(Epipole(F, false));

	Tiny::view.pointSize = 5.0f;
	Tiny::window("Point-Match Reconstruction, Nicholas Mcdonald 2022", 1200, 675);
	Tiny::view.interface = [&](){



	};

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
	cam::pos = vec3(0);
	cam::look = vec3(0,0,1);
	cam::init();

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

/*
	Tiny::view.target(color::black);				//Target Main Screen

	image.use();														//Use Effect Shader
	image.texture("imageTextureA", texA);		//Load Texture
	image.texture("imageTextureB", texB);		//Load Texture
	image.uniform("flip", flip);
	image.uniform("model", flat.model);		//Add Model Matrix
	flat.render();
*/

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
			point3d.uniform("vp", cam::vp);
			pmesh3D.render(GL_POINTS);

		}

	};

	Tiny::loop([&](){

		/*

		vector<vec2> AA, BB;

		for(size_t j = 0; j < 130; j++){
			AA.push_back(A[rand()%A.size()]);
			BB.push_back(B[rand()%B.size()]);
		}

		F = Fundamental(AA, BB);

		vector<vec3> lA; //Epipolar Lines corresponding to points A
		vector<vec3> lB; //Epipolar Lines corresponding to points B

		for(size_t i = 0; i < AA.size(); i++)
			lA.push_back(EpipolarLine(F.transpose(), BB[i]));

		for(size_t i = 0; i < BB.size(); i++)
			lB.push_back(EpipolarLine(F, AA[i]));

		AA.clear();
		BB.clear();
		AA.push_back(Epipole(F, true));
		BB.push_back(Epipole(F, false));

		pbufA.fill(AA);
		pbufB.fill(BB);

		lbufA.fill(lA);
		lbufB.fill(lB);

		usleep(50000);

		*/



	});
	Tiny::quit();

	return 0;
}
