#include <TinyEngine/TinyEngine>
#include <TinyEngine/image>
#include <TinyEngine/color>
#include <TinyEngine/parse>
#include <TinyEngine/camera>

#include "../../../source/triangulate.h"
#include "../../../source/unproject.h"

#include <boost/filesystem.hpp>

using namespace glm;
using namespace std;
using namespace Eigen;

int main( int argc, char* args[] ) {

	if(argc < 5){
		cout<<"This needs at least 4 triangulation files (A, A.warp, B, B.warp)"<<endl;
		exit(0);
	}

	// Launch Window

	Tiny::window("Energy Based Image Triangulation, Nicholas Mcdonald 2022", 960, 540);
	glDisable(GL_CULL_FACE);

	tri::RATIO = 9.6/5.4;

	cam::far = 100.0f;                          //Projection Matrix Far-Clip
	cam::near = 0.001f;
	cam::moverate = 0.05f;
	cam::FOV = 0.5;
	cam::init();
	cam::look = vec3(0,0,0);
	cam::update();

	bool paused = true;
	bool showlines = true;
	bool showA = true;

	Tiny::event.handler = [&](){

		cam::handler();

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_p)
			paused = !paused;

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_n)
			showlines = !showlines;

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_SPACE)
			showA = !showA;

	};

	tri::init();

	// Shaders and Buffers

	Buffer point3Dbuf;
	Buffer point2Dbuf;
	Buffer dist2Dbuf;

	Shader triangle3D({"shader/triangle3D.vs", "shader/triangle3D.fs"}, {"in_Position"}, {"points3D", "index", "colacc"});
	triangle3D.bind<vec4>("points3D", &point3Dbuf);
	triangle3D.bind<ivec4>("index", tri::trianglebuf);
	triangle3D.bind<ivec4>("colacc", tri::tcolaccbuf);

	Shader triangle2D({"shader/triangle2D.vs", "shader/triangle2D.fs"}, {"in_Position"}, {"points2D", "index", "dist2D"});
	triangle2D.bind<vec2>("points2D", &point2Dbuf);
	triangle2D.bind<ivec4>("index", tri::trianglebuf);
	triangle2D.bind<float>("dist2D", &dist2Dbuf);

	Shader point({"shader/point.vs", "shader/point.fs"}, {"in_Position"});

	// Load Triangulations

	tri::triangulation trA, trWA;
	tri::triangulation trB, trWB;

	trA.read(args[1], false);
	trWA.read(args[2], false);
	trB.read(args[3], false);
	trWB.read(args[4], false);

	// Compute the Warped Reprojection Consistency Score

	/*
		We have warpings:
			trA <-> trWA
			trWB <-> trB

		We have consistencies
			trA <-> trWB
			trB <-> trWA

		we can compute a consistency score by:
			warp trA using trWB -> trB, then compute distance to trWA (or reverse, is identical)
			warp trB using trWA -> trA, then compute distance to trWB -""-
	*/

	// Set the Warpings (Note: Origin-Points still the same!)

//	trA.points = trWA.points;
//	trB.points = trWB.points;
//	trWA.originpoints = trA.originpoints;
//	trWB.originpoints = trB.originpoints;

	// Warp the Points

//	trA.reversewarp(trWB.originpoints);
//	trB.reversewarp(trWA.originpoints);

	// Compute the Distance

//	for(size_t i = 0; i < trA.points.size(); i++)
//		cout<<"A "<<length(trWA.originpoints[i] - trA.points[i])<<endl;

//	for(size_t i = 0; i < trB.points.size(); i++)
//		cout<<"B "<<length(trWB.originpoints[i] - trB.points[i])<<endl;

	// Setup 3D Point Buf!!!

	vector<vec4> point3D;

	Triangle triangle;
	Instance triangleinstance(&triangle);

	TLineStrip tlinestrip;
	Instance linestripinstance(&tlinestrip);

	Model pointmesh({"in_Position"});
	pointmesh.bind<vec4>("in_Position", &point3Dbuf);

	// Compute the Reconstruction Set

	vector<vec2> matchX, matchY;

	const mat3 T = mat3(
		0.5f/tri::RATIO, 0.0, 1.0f,
		0.0, -0.5f/tri::RATIO, 1.0f/tri::RATIO,
		0, 0, 1
	);

	vector<vec2> tempX, tempY;
	vector<float> weights;

	vector<float> dist2DA, dist2DB;

	for(size_t i = 0; i < trA.NP; i++){
		vec2 pX = trA.points[i];
		vec2 pY = trWA.points[i];
		tempX.emplace_back(T*vec3(pX.x, pX.y, 1));
		tempY.emplace_back(T*vec3(pY.x, pY.y, 1));
	}

	for(size_t i = 0; i < trB.NP; i++){
		vec2 pX = trB.points[i];
		vec2 pY = trWB.points[i];
	}

	Matrix3f K = unp::Camera();
//	Matrix3f F = unp::F_Sampson(matchX, matchY, weights);
//	Matrix3f F = unp::F_LMEDS(matchX, matchY);
//	Matrix3f F = unp::F_RANSAC(matchX, matchY);


	Matrix3f F;

	F << 16.7481,  270.679, -85.0148,
	-270.653,  4.93792,  116.036,
	 77.8862, -121.158,        1;

//	F << 13.6837,  227.306, -7.45223,
//	-234.242,  4.05449,  61.3264,
//	0.158841,  -61.249,        1;

//	F << 10.686, -274.081,  599.567,
//	345.76, -62.5221, -366.004,
//-601.736,  392.134,        1;

	/*
	F_Sampson:
	F_LMEDS:
	F_RANSACL
	 */


 JacobiSVD<Matrix3f> FS(F, ComputeFullU | ComputeFullV);
 Vector3f S = FS.singularValues();
 S(2) = 0;
 F = FS.matrixU() * S.asDiagonal() * FS.matrixV().transpose();
 cout<<"WHAT THE HELL"<<endl;
 cout<<F<<endl;
 FS.compute(F, ComputeFullU | ComputeFullV);
 cout<<FS.singularValues()<<endl;


	cout<<"Fundamental Matrix: "<<F<<endl;

	point3D = unp::triangulate(F, K, tempX, tempY);
	point3Dbuf.fill(point3D);

	tri::upload(&trA);

	int NT;


	Tiny::view.interface = [&](){

		ImGui::SetNextWindowSize(ImVec2(480, 260), ImGuiCond_Once);
//		ImGui::SetNextWindowPos(ImVec2(50, 470), ImGuiCond_Once);

		ImGui::Begin("F Computer Controller", NULL, 0);

		ImGui::DragFloat("px", &unp::px, 0.01f, -5.0f, 5.0f);
		ImGui::DragFloat("py", &unp::py, 0.01f, -5.0f, 5.0f);
		ImGui::DragFloat("fx", &unp::fx, 0.01f, -5.0f, 5.0f);
		ImGui::DragInt("check", &unp::check, 1, 0, 3);

		if(ImGui::Button("Reproject")){
			K = unp::Camera();
			point3D = unp::triangulate(F, K, tempX, tempY);
			point3Dbuf.fill(point3D);
		}

		ImGui::End();

	};


	Tiny::view.pipeline = [&](){

		Tiny::view.target(color::black);				//Target Main Screen

		triangle2D.use();
		triangle2D.uniform("RATIO", tri::RATIO);

		if(showlines){

			if(showA){

				dist2Dbuf.fill(dist2DA);

				point2Dbuf.fill(trA.points);
				tri::trianglebuf->fill(trA.triangles);
				linestripinstance.render(GL_LINE_STRIP, trA.NT);

				point2Dbuf.fill(trWA.originpoints);
				tri::trianglebuf->fill(trWA.triangles);
				linestripinstance.render(GL_LINE_STRIP, trWA.NT);

				NT = trA.NT;

			}

			else {

				dist2Dbuf.fill(dist2DB);

				point2Dbuf.fill(trB.points);
				tri::trianglebuf->fill(trB.triangles);
				linestripinstance.render(GL_LINE_STRIP, trB.NT);

				point2Dbuf.fill(trWB.originpoints);
				tri::trianglebuf->fill(trWB.triangles);
				linestripinstance.render(GL_LINE_STRIP, trWB.NT);

				NT = trB.NT;

			}

		}

		else {

			triangle3D.use();
			triangle3D.uniform("RATIO", tri::RATIO);
			triangle3D.uniform("vp", cam::vp);
			triangle3D.uniform("model", rotate(mat4(1.0f), 3.14159265f, vec3(0,0,1)));
			triangleinstance.render(GL_TRIANGLE_STRIP, NT);

		}

	};

	Tiny::loop([&](){});

	tri::quit();
	Tiny::quit();

	return 0;

}
