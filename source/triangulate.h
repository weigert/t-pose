#ifndef TPOSE_TRIANGULATE
#define TPOSE_TRIANGULATE

// triangulate.h

#include <fstream>
#include "include/delaunator-cpp/delaunator-header-only.hpp"
#include "include/poisson.h"
#include "utility.h"

namespace tri {

using namespace glm;
using namespace std;

float RATIO = 12.0f/8.0f;
const float PI = 3.14159265f;

/*
================================================================================
														Triangulation Struct
================================================================================
*/

struct triangulation {

	// Main Members

	vector<vec2> points;
	vector<vec2> originpoints;
	vector<ivec4> colors;
	vector<ivec4> triangles;
	vector<int> halfedges;
	vector<int> sizes;

	int NP;								// Number of Points
	int NT;								// Number of Triangles
	static size_t MAXT;		// Maximum Number of Triangles

	// Initial Conditions

	triangulation( string filename ){

		read(filename, false);

	}

	triangulation( size_t K = 0 ){

		points.emplace_back(-RATIO,-1);
		points.emplace_back(-RATIO, 1);
		points.emplace_back( RATIO,-1);
		points.emplace_back( RATIO, 1);

		while(points.size() < K/2)
			sample::disc(points, K, vec2(-RATIO, -1.0f), vec2(RATIO, 1.0f));

		vector<double> coords;					//Coordinates for Delaunation
	  for(size_t i = 0; i < points.size(); i++){
	    coords.push_back(points[i].x);
	    coords.push_back(points[i].y);
	  }

		delaunator::Delaunator d(coords);			//Compute Delaunay Triangulation

	  for(size_t i = 0; i < d.triangles.size()/3; i++){
			triangles.emplace_back(d.triangles[3*i+0], d.triangles[3*i+1], d.triangles[3*i+2], 0);
			halfedges.push_back(d.halfedges[3*i+0]);
			halfedges.push_back(d.halfedges[3*i+1]);
			halfedges.push_back(d.halfedges[3*i+2]);
			colors.emplace_back(0,0,0,1);
			sizes.push_back(0);
		}

		NP = points.size();
		NT = triangles.size();
		colors.resize(MAXT);

	}

	// Helper Functions

	float angle( int ha );
	float hlength( int ha );
	static bool boundary( vec2 p );
	int boundary( int t );

	// Topological Modifications

	bool prune( int ta );				// Prune Boundary Triangle
	bool flip( int ha );				// Flip Triangulation Edge
	bool collapse( int ha );		// Collapse Triangulation Edge
	bool split( int ta );				// Split Triangle
	bool optimize();						// Optimization Procedure Wrapper

	// IO Functions

	void read( string file, bool warp );
	void write( string file, bool normalize );

};

size_t triangulation::MAXT = (2 << 18);

/*
================================================================================
											Triangulation Helper Functions
================================================================================
*/

// Angle Opposite to Half-Edge

float triangulation::angle( int ha ){

	int ta = ha/3;	// Triangle Index

	vec2 paa = points[triangles[ta][(ha+0)%3]];
	vec2 pab = points[triangles[ta][(ha+1)%3]];
	vec2 pac = points[triangles[ta][(ha+2)%3]];
	if(length(paa - pac) == 0) return 0;
	if(length(pab - pac) == 0) return 0;
	return acos(dot(paa - pac, pab - pac) / length(paa - pac) / length(pab - pac));

}

// Length of Half-Edge

float triangulation::hlength( int ha ){

	int ta = ha/3;	// Triangle Index

	vec2 paa = points[triangles[ta][(ha+0)%3]];
	vec2 pab = points[triangles[ta][(ha+1)%3]];
	return length(pab - paa);

}

// Number of Vertices on Boundary (Triangle)

bool triangulation::boundary( vec2 p ){

	if(p.x <= -RATIO
	|| p.y <= -1
	|| p.x >=  RATIO
	|| p.y >=  1) return true;
	return false;

}

int triangulation::boundary( int t ){

	int nboundary = 0;

	if(triangulation::boundary(points[triangles[t].x]))
		nboundary++;
	if(triangulation::boundary(points[triangles[t].y]))
		nboundary++;
	if(triangulation::boundary(points[triangles[t].z]))
		nboundary++;

	return nboundary;

}


/*
================================================================================
									Triangulation Topological Alterations
================================================================================
*/

// Prune Triangle from Triangulation Boundary

bool triangulation::prune( int ta ){

	// Get Exterior Half-Edges

	int ta0 = halfedges[3*ta+0];
	int ta1 = halfedges[3*ta+1];
	int ta2 = halfedges[3*ta+2];

	// Check Non-Prunable

	if(ta0 >= 0 && ta1 >= 0 && ta2 >= 0)
		return false;

	if(angle(3*ta+0) > 0 && angle(3*ta+0) < PI) return false;
	if(angle(3*ta+1) > 0 && angle(3*ta+1) < PI) return false;
	if(angle(3*ta+2) > 0 && angle(3*ta+2) < PI) return false;

	// Remove References

	if(ta0 >= 0) halfedges[ta0] = -1;
	if(ta1 >= 0) halfedges[ta1] = -1;
	if(ta2 >= 0) halfedges[ta2] = -1;

	// Delete the Triangle
	// Note that we don't actually delete any vertices!

	triangles.erase(triangles.begin() + ta);
	halfedges.erase(halfedges.begin() + 3*ta, halfedges.begin() + 3*(ta + 1));

	// Fix the Indexing!

	for(auto& h: halfedges){

		int th = h;
		if(th >= 3*(ta+1)) h -= 3;

	}

	NT--;

	cout<<"PRUNED"<<endl;
	return true;

}

// Delaunay Flipping

bool triangulation::flip( int ha ){

	int hb = halfedges[ha];		// Opposing Half-Edge
	int ta = ha/3;						// First Triangle
	int tb = hb/3;						// Second Triangle

	if(hb < 0)	return false;	// No Opposing Half-Edge

	float aa = angle(ha);
	float ab = angle(hb);

	if(aa + ab < PI)
		return false;

	if(aa <= 1E-8 || ab <= 1E-8)
		return false;

	// Retrieve Half-Edge Indices

	int ta0 = halfedges[3*ta+(ha+0)%3];
	int ta1 = halfedges[3*ta+(ha+1)%3];
	int ta2 = halfedges[3*ta+(ha+2)%3];

	int tb0 = halfedges[3*tb+(hb+0)%3];
	int tb1 = halfedges[3*tb+(hb+1)%3];
	int tb2 = halfedges[3*tb+(hb+2)%3];

	// Retrieve Triangle Vertex Indices

	ivec4 tca = triangles[ta];
	ivec4 tcb = triangles[tb];

	// Interior Half-Edge Reference Shift

	halfedges[3*ta+(ha+0)%3] = ta0;
	halfedges[3*ta+(ha+1)%3] = ta2;
	halfedges[3*ta+(ha+2)%3] = tb1;

	halfedges[3*tb+(hb+0)%3] = tb0;
	halfedges[3*tb+(hb+1)%3] = tb2;
	halfedges[3*tb+(hb+2)%3] = ta1;

	// Exterior Half-Edge Reference Shift

	if(ta1 >= 0) halfedges[ta1] = 3*tb+(hb+2)%3;
	if(ta2 >= 0) halfedges[ta2] = 3*ta+(ha+1)%3;

	if(tb1 >= 0) halfedges[tb1] = 3*ta+(ha+2)%3;
	if(tb2 >= 0) halfedges[tb2] = 3*tb+(hb+1)%3;

	// Vertex Shift

	triangles[ta][(ha+0)%3] = tcb[(hb+2)%3];
	triangles[ta][(ha+1)%3] = tca[(ha+2)%3];
	triangles[ta][(ha+2)%3] = tcb[(hb+1)%3];

	triangles[tb][(hb+0)%3] = tca[(ha+2)%3];
	triangles[tb][(hb+1)%3] = tcb[(hb+2)%3];
	triangles[tb][(hb+2)%3] = tca[(ha+1)%3];

	//cout<<"FLIPPED"<<endl;
	return true;

}

// Half-Edge Collapse

bool triangulation::collapse( int ha ){

	int hb = halfedges[ha];		// Opposing Half-Edge
	int ta = ha/3;						// First Triangle
	int tb = hb/3;						// Second Triangle

	if(hb < 0)	return false; // No Opposing Half-Edge

	int ia = triangles[ta][(ha+0)%3];
	int ib = triangles[tb][(hb+0)%3];

	if(length(points[ia] - points[ib]) > 0.01)
		return false;

	// Add new Point, with boundary checking!

	vec2 vn;
	if(boundary(points[ia]) && boundary(points[ib]))
		vn = 0.5f*(points[ia] + points[ib]);
	else if(boundary(points[ia])) vn = points[ia];
	else if(boundary(points[ib])) vn = points[ib];
	else 	vn = 0.5f*(points[ia] + points[ib]);

	int in = points.size();
	points.push_back(vn);

	// Adjust all Assignments to the Vertices

	for(auto& t: triangles){

		if(t.x == ia || t.x == ib) t.x = in;
		if(t.y == ia || t.y == ib) t.y = in;
		if(t.z == ia || t.z == ib) t.z = in;

	}

	// Retrieve Exterior Half-Edge Indices

	int ta1 = halfedges[3*ta+(ha+1)%3];
	int ta2 = halfedges[3*ta+(ha+2)%3];

	int tb1 = halfedges[3*tb+(hb+1)%3];
	int tb2 = halfedges[3*tb+(hb+2)%3];

	// Adjust Exterior Half-Edges

	if(ta1 >= 0) halfedges[ta1] = ta2;
	if(ta2 >= 0) halfedges[ta2] = ta1;

	if(tb1 >= 0) halfedges[tb1] = tb2;
	if(tb2 >= 0) halfedges[tb2] = tb1;

	// Erase the Points, Halfedges, Triangles

	if(ta > tb){

		triangles.erase(triangles.begin() + ta);
		triangles.erase(triangles.begin() + tb);

		halfedges.erase(halfedges.begin() + 3*ta, halfedges.begin() + 3*(ta + 1));
		halfedges.erase(halfedges.begin() + 3*tb, halfedges.begin() + 3*(tb + 1));

	}

	else {

		triangles.erase(triangles.begin() + tb);
		triangles.erase(triangles.begin() + ta);

		halfedges.erase(halfedges.begin() + 3*tb, halfedges.begin() + 3*(tb + 1));
		halfedges.erase(halfedges.begin() + 3*ta, halfedges.begin() + 3*(ta + 1));

	}

	if(ia > ib){

		points.erase(points.begin()+ia);
		points.erase(points.begin()+ib);

	}

	else {

		points.erase(points.begin()+ib);
		points.erase(points.begin()+ia);

	}

	// Fix the Indexing!

	for(auto& t: triangles){

		ivec4 tt = t;
		if(tt.x >= ia) t.x--;
		if(tt.y >= ia) t.y--;
		if(tt.z >= ia) t.z--;
		if(tt.x >= ib) t.x--;
		if(tt.y >= ib) t.y--;
		if(tt.z >= ib) t.z--;

	}

	for(auto& h: halfedges){

		int th = h;
		if(th >= 3*(ta+1)) h -= 3;
		if(th >= 3*(tb+1)) h -= 3;

	}

	NT -= 2;
	NP -= 1;

	cout<<"COLLAPSED"<<endl;
	return true;

}

// Triangle Centroid Splitting

bool triangulation::split( int ta ){

	ivec4 tca = triangles[ta];	//Triangle Vertices

	// Compute Centroid, Add Centroid to Triangulation

	vec2 centroid = (points[tca.x] + points[tca.y] + points[tca.z])/3.0f;
	int nind = points.size();
	points.push_back(centroid);

	// Original Exterior Half-Edges

	int tax = halfedges[3*ta + 0];
	int tay = halfedges[3*ta + 1];
	int taz = halfedges[3*ta + 2];

	// New Triangle Indices, Add new Triangles, Adjust Original

	int tb = triangles.size();
	int tc = tb + 1;

	triangles.push_back(ivec4(tca.y, tca.z, nind, 0));
	triangles.push_back(ivec4(tca.z, tca.x, nind, 0));
	triangles[ta].z = nind;

	// Interior Half-Edge Reference Shift

	halfedges[3*ta + 0] = tax;
	halfedges[3*ta + 1] = 3*tb + 2;
	halfedges[3*ta + 2] = 3*tc + 1;

	halfedges.push_back(tay);
	halfedges.push_back(3*tc + 2);
	halfedges.push_back(3*ta + 1);

	halfedges.push_back(taz);
	halfedges.push_back(3*ta + 2);
	halfedges.push_back(3*tb + 1);

	// Exterior Half-Edge Reference Shift

	if(tax >= 0)	halfedges[tax] = 3*ta + 0;
	if(tay >= 0)	halfedges[tay] = 3*tb + 0;
	if(taz >= 0)	halfedges[taz] = 3*tc + 0;

	// Update Numbers

	NT += 2;
	NP += 1;

//	cout<<"SPLIT "<<ta<<endl;
	return true;

}

/*
================================================================================
											Topological Optimization Strategy
================================================================================

	Idea for an Optimization Strategy:
		- Wait till the energy stops sinking
		- Merge Faces which are too close
		- Flip until we can't anymore
		- Compute the energy and split expensive faces
		- Repeat

*/

bool triangulation::optimize(){

	// cout<<"OPTIMIZE"<<endl;

	// Prune Flat Boundary Triangles

	for(size_t ta = 0; ta < NT; ta++)
	if(boundary(ta) == 3)
		prune(ta);

	// Attempt a Delaunay Flip on a Triangle's Largest Angle

	for(size_t ta = 0; ta < NT; ta++){

		int ha = 3*ta + 0;
		float maxangle = angle( ha );
		if(angle( ha + 1 ) > maxangle)
			maxangle = angle( ++ha );
		if(angle( ha + 1 ) > maxangle)
			maxangle = angle( ++ha );
		flip(ha);

	}

	// Collapse Small Edges

	for(size_t ta = 0; ta < triangles.size(); ta++){

		int ha = 3*ta + 0;
		float minlength = hlength( ha );
		if(hlength( ha + 1 ) < minlength)
			minlength = hlength( ++ha );
		if(hlength( ha + 1 ) < minlength)
			minlength = hlength( ++ha );
		collapse(ha);

	}

	return true;

}

// Rendering GPU Buffers

Buffer* trianglebuf;
Buffer* pointbuf;
Buffer* tcolaccbuf;
Buffer* tcolnumbuf;
Buffer* tenergybuf;	//Triangle Surface Energy
Buffer* penergybuf;	//Vertex Tension Energy
Buffer* pgradbuf;
Buffer* tnringbuf;

int* terr;
int* perr;
int* cn;
glm::ivec4* col;

void init(){

	pointbuf 		= new Buffer( tri::triangulation::MAXT, (glm::vec2*)NULL );
	trianglebuf = new Buffer( tri::triangulation::MAXT, (glm::ivec4*)NULL );
	tcolaccbuf 	= new Buffer( tri::triangulation::MAXT, (glm::ivec4*)NULL );		// Raw Color
	tcolnumbuf 	= new Buffer( tri::triangulation::MAXT, (int*)NULL );			// Triangle Size (Pixels)
	tenergybuf 	= new Buffer( tri::triangulation::MAXT, (int*)NULL );			// Triangle Energy
	penergybuf 	= new Buffer( tri::triangulation::MAXT, (int*)NULL );			// Vertex Energy
	pgradbuf 		= new Buffer( tri::triangulation::MAXT, (glm::ivec2*)NULL );
	tnringbuf 	= new Buffer( tri::triangulation::MAXT, (int*)NULL );

	terr = new int[tri::triangulation::MAXT];
	perr = new int[tri::triangulation::MAXT];
	cn 	= new int[tri::triangulation::MAXT];
	col = new glm::ivec4[tri::triangulation::MAXT];

}

void quit(){

	delete pointbuf;
	delete trianglebuf;
	delete tcolaccbuf;
	delete tcolnumbuf;
	delete tenergybuf;
	delete penergybuf;
	delete pgradbuf;
	delete tnringbuf;

	delete[] terr;
	delete[] perr;
	delete[] cn;
	delete[] col;

}

void upload( tri::triangulation* tr, bool uploadcolor = true ){

	pointbuf->fill(tr->points);
	trianglebuf->fill(tr->triangles);

	if(uploadcolor){
		int nc = 0;
		for(auto& c: tr->colors){
			for(size_t i = 0; i < 13; i++)
				col[i*tr->NT + nc] = c;
			nc++;
		}
		tcolaccbuf->fill(13*tr->NT, col);
	}

}

// Error Computations


float toterr = 1.0f;
float newerr;
float relerr;
float maxerr;

float geterr( tri::triangulation* tr ){

	maxerr = 0.0f;
	newerr = 0.0f;

	for(size_t i = 0; i < tr->NT; i++){
		float err = 0.0f;
		err += terr[i];
	//	err += perr[tr->triangles[i].x];
	//	err += perr[tr->triangles[i].y];
	//	err += perr[tr->triangles[i].z];
		if(sqrt(err) >= maxerr)
			maxerr = sqrt(err);
		newerr += err;
	}

	relerr = (toterr - newerr)/toterr;
	toterr = newerr;

	return abs(relerr);

}

int maxerrid( tri::triangulation* tr ){

	maxerr = 0;
	int tta = -1;
	for(size_t i = 0; i < tr->NT; i++){
		if(cn[i] == 0) continue;
		if(cn[i] <= 50) continue;
		float err = 0.0f;
		err += terr[i];
		err += perr[tr->triangles[i].x];
		err += perr[tr->triangles[i].y];
		err += perr[tr->triangles[i].z];
		if(sqrt(err) > maxerr){
			maxerr = sqrt(err);
			tta = i;
		}
	}

	return tta;

}

/*
================================================================================
														Triangulation IO
================================================================================
*/

// Export a Triangulation

void triangulation::write( string file, bool normalize = true ){

	cout<<"Exporting to file "<<file<<endl;
	ofstream out(file, ios::out);
	if(!out.is_open()){
		cout<<"Failed to open file "<<file<<endl;
		exit(0);
	}

	// Export Vertices

	out<<"VERTEX"<<endl;
	for(auto& p: points)
		out<<p.x<<" "<<p.y<<endl;

	// Export Halfedges

	out<<"HALFEDGE"<<endl;
	for(size_t i = 0; i < halfedges.size()/3; i++)
		out<<halfedges[3*i+0]<<" "<<halfedges[3*i+1]<<" "<<halfedges[3*i+2]<<endl;

	// Export Triangles

	out<<"TRIANGLE"<<endl;
	for(auto& t: triangles)
		out<<t.x<<" "<<t.y<<" "<<t.z<<endl;

	// Export Colors

	out<<"COLOR"<<endl;
	for(size_t i = 0; i < NT; i++){
		if(normalize){
			if(cn[i] == 0) cn[i] = 1;
			out<<colors[i].x/cn[i]<<" "<<colors[i].y/cn[i]<<" "<<colors[i].z/cn[i]<<endl;
		}
		else out<<colors[i].x<<" "<<colors[i].y<<" "<<colors[i].z<<endl;

	}

	out.close();

}

// Import a Triangulation

void triangulation::read( string file, bool warp = true ){

	cout<<"Importing from file "<<file<<endl;
	ifstream in(file, ios::in);
	if(!in.is_open()){
		cout<<"Failed to open file "<<file<<endl;
		exit(0);
	}

	string pstring;

	vec2 p = vec2(0);
	ivec4 c = ivec4(0);
	ivec4 t = ivec4(0);
	ivec3 h = ivec3(0);

	vector<vec2> npoints;
	vector<vec2> noriginpoints;
	while(!in.eof()){

		getline(in, pstring);
		if(in.eof())
			break;

		if(pstring == "HALFEDGE")
			break;

		int m = sscanf(pstring.c_str(), "%f %f", &p.x, &p.y);
		if(m == 2){
			npoints.push_back(p);
			noriginpoints.push_back(p);
		}

	}

	// Check for Warping

	if(!triangles.empty() && warp)
	for(size_t i = 0; i < npoints.size(); i++){		//Iterate over new Points

		if(boundary(npoints[i]))
			continue;

		for(auto& t: triangles){

			if(!intriangle(npoints[i], t, originpoints))
				continue;

			npoints[i] = cartesian(barycentric(npoints[i], t, originpoints), t, points);
			break;

		}

	}

	points = npoints;
	originpoints = noriginpoints;
	NP = points.size();

	halfedges.clear();
	while(!in.eof()){

		getline(in, pstring);
		if(in.eof())
			break;

		if(pstring == "TRIANGLE")
			break;

		int m = sscanf(pstring.c_str(), "%u %u %u", &h.x, &h.y, &h.z);
		if(m == 3){
			halfedges.push_back(h.x);
			halfedges.push_back(h.y);
			halfedges.push_back(h.z);
		}

	}

	triangles.clear();
	while(!in.eof()){

		getline(in, pstring);
		if(in.eof())
			break;

		if(pstring == "COLOR")
			break;

		int m = sscanf(pstring.c_str(), "%u %u %u", &t.x, &t.y, &t.z);
		if(m == 3) triangles.push_back(t);

	}

	NT = triangles.size();

	colors.clear();
	while(!in.eof()){

		getline(in, pstring);
		if(in.eof())
			break;

		int m = sscanf(pstring.c_str(), "%u %u %u", &c.x, &c.y, &c.z);
		if(m == 3) colors.push_back(ivec4(c.x, c.y, c.z, 1));

	}

	in.close();

}


} //End of Namespace

/*
================================================================================
												TinyEngine Rendering Interface
================================================================================
*/

// Rendering Structures

struct Triangle : Model {
	Buffer vert;
	Triangle():Model({"vert"}),
	vert({1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f}){
		bind<glm::vec3>("vert", &vert);
		SIZE = 3;
	}
};

struct TLineStrip : Model {
	Buffer vert;
	TLineStrip():Model({"vert"}),
	vert({1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f}){
		bind<glm::vec3>("vert", &vert);
		SIZE = 4;
	}
};

#endif
