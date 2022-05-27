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

	tri::init();

	// Shaders and Buffers

	Buffer point2Dbuf_from;
	Buffer point2Dbuf_to;

	Buffer selectedbuf;

	Shader triangle2D({"shader/triangle2D.vs", "shader/triangle2D.fs"}, {"in_Position"}, {"index", "colacc", "points2Dfrom", "points2Dto", "selected"});
	triangle2D.bind<ivec4>("index", tri::trianglebuf);
	triangle2D.bind<ivec4>("colacc", tri::tcolaccbuf);
	triangle2D.bind<vec2>("points2Dfrom", &point2Dbuf_from);
	triangle2D.bind<vec2>("points2Dto", &point2Dbuf_to);
	triangle2D.bind<int>("selected", &selectedbuf);

	tri::triangulation trA, trWA;
	tri::triangulation trB, trWB;

	vector<int> selectedA;
	vector<int> selectedB;

	// Load Triangulations

	trA.read(args[1], false);
	trWA.read(args[2], false);
	trB.read(args[3], false);
	trWB.read(args[4], false);

	// Set the Warpings (Note: Origin-Points still the same!)

	trA.points = trWA.points;
	trB.points = trWB.points;
	trWA.originpoints = trA.originpoints;
	trWB.originpoints = trB.originpoints;

	trA.reversewarp(trWB.originpoints);
	trB.reversewarp(trWA.originpoints);

	for(size_t i = 0; i < trA.NT; i++)
		selectedA.push_back(0);

	for(size_t i = 0; i < trB.NT; i++)
		selectedB.push_back(0);

	// Rendering Objects

	Triangle triangle;
	Instance triangleinstance(&triangle);

	TLineStrip tlinestrip;
	Instance linestripinstance(&tlinestrip);





	float warp = 0.0f;
	int grow = 0;

	Tiny::view.interface = [&](){

		ImGui::SetNextWindowSize(ImVec2(480, 260), ImGuiCond_Once);
		ImGui::SetNextWindowPos(ImVec2(50, 470), ImGuiCond_Once);

		ImGui::Begin("F Computer Controller", NULL, ImGuiWindowFlags_NoResize);

		ImGui::DragFloat("Warp", &warp, 0.01f, 0.0f, 1.0f);
		ImGui::DragInt("Grow", &grow, 1, 0, 10);

			if(ImGui::Button("Compute F")){

				cout<<"Computing Fundamental Matrix..."<<endl;

				// Basically, compute the set of points which are eligible!

				vector<int> pointsA, pointsB;

				for(size_t i = 0; i < trA.points.size(); i++)
					pointsA.push_back(0);

				for(size_t i = 0; i < trB.points.size(); i++)
					pointsB.push_back(0);

				// Find all used points

				for(size_t i = 0; i < trA.triangles.size(); i++){
					if(selectedA[i] == 0) continue;
					pointsA[ trA.triangles[i].x ] = 1;
					pointsA[ trA.triangles[i].y ] = 1;
					pointsA[ trA.triangles[i].z ] = 1;
				}

				for(size_t i = 0; i < trB.triangles.size(); i++){
					if(selectedB[i] == 0) continue;
					pointsB[ trB.triangles[i].x ] = 1;
					pointsB[ trB.triangles[i].y ] = 1;
					pointsB[ trB.triangles[i].z ] = 1;
				}

				// Count the number of points, set up match vectors

				int NPA = 0;
				int NPB = 0;

				const mat3 T = mat3(
					0.5f/tri::RATIO, 0.0, 1.0f,
					0.0, -0.5f/tri::RATIO, 1.0f/tri::RATIO,
					0, 0, 1
				);

				vector<vec2> matchX, matchY;

				for(size_t i = 0; i < pointsA.size(); i++)
				if(pointsA[i]){
					NPA++;

					vec2 pX = trA.originpoints[i];
					vec2 pY = trA.points[i];

					matchX.emplace_back(T*vec3(pX.x, pX.y, 1));
					matchY.emplace_back(T*vec3(pY.x, pY.y, 1));

				}

				for(size_t i = 0; i < pointsB.size(); i++)
				if(pointsB[i]){
					NPB++;

					vec2 pX = trB.points[i];
					vec2 pY = trB.originpoints[i];

					matchX.emplace_back(T*vec3(pX.x, pX.y, 1));
					matchY.emplace_back(T*vec3(pY.x, pY.y, 1));

				}

				cout<<"Found A Matches: "<<NPA<<endl;
				cout<<"Found B Matches: "<<NPB<<endl;

				if( matchX.size() < 8 ){

					cout<<"Insufficient Matches..."<<endl;

				}

				else {

					// Compute Fundamental Matrices

					cout<<"F_Sampson: "<<unp::F_Sampson(matchX, matchY)<<endl;
					cout<<"F_LMEDS: "<<unp::F_LMEDS(matchX, matchY)<<endl;
					cout<<"F_RANSACL "<<unp::F_RANSAC(matchX, matchY)<<endl;

					// Create output triangulation from the matches???

					tri::triangulation pruneA, pruneWA;
					tri::triangulation pruneB, pruneWB;

					pruneA.triangles.clear();
					pruneA.halfedges.clear();
					pruneA.colors.clear();
					pruneA.points.clear();

					pruneWA.triangles.clear();
					pruneWA.halfedges.clear();
					pruneWA.colors.clear();
					pruneWA.points.clear();

					pruneB.triangles.clear();
					pruneB.halfedges.clear();
					pruneB.colors.clear();
					pruneB.points.clear();

					pruneWB.triangles.clear();
					pruneWB.halfedges.clear();
					pruneWB.colors.clear();
					pruneWB.points.clear();

					for(size_t i = 0; i < trA.triangles.size(); i++){
						if(selectedA[i] == 0) continue;

						pruneA.triangles.push_back( trA.triangles[i] );
						pruneA.halfedges.push_back( -1 );
						pruneA.halfedges.push_back( -1 );
						pruneA.halfedges.push_back( -1 );

						pruneWA.triangles.push_back( trA.triangles[i] );
						pruneWA.halfedges.push_back( -1 );
						pruneWA.halfedges.push_back( -1 );
						pruneWA.halfedges.push_back( -1 );

						pruneA.colors.push_back( trA.colors[i] );
						pruneWA.colors.push_back( trA.colors[i] );

					}

					int S = 0;


					for(size_t i = 0; i < pointsA.size(); i++)
					if(pointsA[i]){

						pruneA.points.push_back( trA.originpoints[i] );
						pruneWA.points.push_back( trA.points[i] );

					}
					else {

						for(size_t j = 0; j < pruneA.triangles.size(); j++){
							if(pruneA.triangles[j].x > i+S) pruneA.triangles[j].x--;
							if(pruneA.triangles[j].y > i+S) pruneA.triangles[j].y--;
							if(pruneA.triangles[j].z > i+S) pruneA.triangles[j].z--;
						}

						for(size_t j = 0; j < pruneWA.triangles.size(); j++){
							if(pruneWA.triangles[j].x > i+S) pruneWA.triangles[j].x--;
							if(pruneWA.triangles[j].y > i+S) pruneWA.triangles[j].y--;
							if(pruneWA.triangles[j].z > i+S) pruneWA.triangles[j].z--;
						}

						S--;

					}

					for(size_t i = 0; i < trB.triangles.size(); i++){
						if(selectedB[i] == 0) continue;

						pruneB.triangles.push_back( trB.triangles[i] );
						pruneB.halfedges.push_back( -1 );
						pruneB.halfedges.push_back( -1 );
						pruneB.halfedges.push_back( -1 );

						pruneWB.triangles.push_back( trB.triangles[i] );
						pruneWB.halfedges.push_back( -1 );
						pruneWB.halfedges.push_back( -1 );
						pruneWB.halfedges.push_back( -1 );

						pruneB.colors.push_back( trB.colors[i] );
						pruneWB.colors.push_back( trB.colors[i] );

					}

					S = 0;

					for(size_t i = 0; i < pointsB.size(); i++)
					if(pointsB[i]){

						pruneB.points.push_back( trB.originpoints[i] );
						pruneWB.points.push_back( trB.points[i] );

					}
					else {

						for(size_t j = 0; j < pruneB.triangles.size(); j++){
							if(pruneB.triangles[j].x > i+S) pruneB.triangles[j].x--;
							if(pruneB.triangles[j].y > i+S) pruneB.triangles[j].y--;
							if(pruneB.triangles[j].z > i+S) pruneB.triangles[j].z--;
						}

						for(size_t j = 0; j < pruneWB.triangles.size(); j++){
							if(pruneWB.triangles[j].x > i+S) pruneWB.triangles[j].x--;
							if(pruneWB.triangles[j].y > i+S) pruneWB.triangles[j].y--;
							if(pruneWB.triangles[j].z > i+S) pruneWB.triangles[j].z--;
						}

						S--;

					}

					pruneA.NT = pruneA.triangles.size();
					pruneWA.NT = pruneWA.triangles.size();
					pruneB.NT = pruneB.triangles.size();
					pruneWB.NT = pruneWB.triangles.size();

					pruneA.write("A.tri", false);
					pruneWA.write("AW.tri", false);
					pruneB.write("B.tri", false);
					pruneWB.write("BW.tri", false);

				}

			}

		ImGui::End();

	};

	bool showA = true;

	Tiny::event.handler = [&](){

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_SPACE)
			showA = !showA;

		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_d){

			// Find the triangle which was clicked!
			tri::triangulation* tr;
			if(showA) tr = &trA;
			else tr = &trB;

			vec2 p = vec2(Tiny::event.mouse.x, Tiny::event.mouse.y);
			p /= vec2(960, 540);
			p *= 2.0f;
			p -= 1.0f;
			p.x *= tri::RATIO;
			p.y *= -1;

			cout<<p.x<<" "<<p.y<<endl;

			for(size_t i = 0; i < tr->triangles.size(); i++)
			if(intriangle(p, tr->triangles[i], tr->originpoints, tr->points, (showA)?warp:1-warp)){

				if(showA) selectedA[i] = 1 - selectedA[i];
				else selectedB[i] = 1 - selectedB[i];

				// do grow levels of recursion??

				const function<void(int, int)> dogrow = [&](int h, int d){

					if(d <= 0) return;

					if(showA){
						selectedA[h/3] = selectedA[i];
						dogrow(tr->halfedges[ 3*(h/3) + (h+1)%3 ], d-1);
						dogrow(tr->halfedges[ 3*(h/3) + (h+2)%3 ], d-1);
					//	if(selectedA[tr->halfedges[ 3*t + 0 ]/3] != selectedA[i]) dogrow(tr->halfedges[ 3*t + 0 ]/3, d-1);
					//	if(selectedA[tr->halfedges[ 3*t + 1 ]/3] != selectedA[i]) dogrow(tr->halfedges[ 3*t + 1 ]/3, d-1);
					//	if(selectedA[tr->halfedges[ 3*t + 2 ]/3] != selectedA[i]) dogrow(tr->halfedges[ 3*t + 2 ]/3, d-1);
					}

					else{
						selectedB[h/3] = selectedB[i];
						dogrow(tr->halfedges[ 3*(h/3) + (h+1)%3 ], d-1);
						dogrow(tr->halfedges[ 3*(h/3) + (h+2)%3 ], d-1);
					//	if(selectedB[tr->halfedges[ 3*t + 0 ]/3] != selectedB[i]) dogrow(tr->halfedges[ 3*t + 0 ]/3, d-1);
					//	if(selectedB[tr->halfedges[ 3*t + 1 ]/3] != selectedB[i]) dogrow(tr->halfedges[ 3*t + 1 ]/3, d-1);
					//	if(selectedB[tr->halfedges[ 3*t + 2 ]/3] != selectedB[i]) dogrow(tr->halfedges[ 3*t + 2 ]/3, d-1);
					}

				};

				dogrow(3*i+0, grow);
				break;

			}

		}

	};


	Tiny::view.pipeline = [&](){

		Tiny::view.target(color::black);				//Target Main Screen

		triangle2D.use();
		triangle2D.uniform("RATIO", tri::RATIO);

		if(showA){

			point2Dbuf_from.fill(trA.originpoints);
			point2Dbuf_to.fill(trA.points);
			selectedbuf.fill(selectedA);
			tri::trianglebuf->fill(trA.triangles);
			tri::tcolaccbuf->fill(trA.colors);

			triangle2D.uniform("docolor", true);
			triangleinstance.render(GL_TRIANGLES, trA.NT);

			triangle2D.uniform("warp", warp);
			triangle2D.uniform("docolor", false);
			linestripinstance.render(GL_LINE_STRIP, trA.NT);

		}

		else {

			point2Dbuf_from.fill(trB.originpoints);
			point2Dbuf_to.fill(trB.points);
			selectedbuf.fill(selectedB);
			tri::trianglebuf->fill(trB.triangles);
			tri::tcolaccbuf->fill(trB.colors);

			triangle2D.uniform("docolor", true);
			triangleinstance.render(GL_TRIANGLES, trB.NT);

			triangle2D.uniform("warp", 1.0f-warp);
			triangle2D.uniform("docolor", false);
			linestripinstance.render(GL_LINE_STRIP, trB.NT);

		}

	};

	Tiny::loop([&](){});

	tri::quit();
	Tiny::quit();

	return 0;

}
