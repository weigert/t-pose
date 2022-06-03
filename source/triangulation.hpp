/*
================================================================================
													t-pose: triangulation.h
================================================================================

This header defines a half-edge based triangulation struct, which includes
operations for altering its topology and executing queries about its structure.

*/

#ifndef TPOSE_TRIANGULATION
#define TPOSE_TRIANGULATION

#include <tpose/utility>
#include <fstream>

namespace tpose {

using namespace glm;
using namespace std;

float RATIO = 12.0f/8.0f;
const float PI = 3.14159265f;

// Main Triangulation Struct

struct triangulation {

	static size_t MAXT;					// Maximum Number of Triangles

	int NT;											// Number of Triangles
	vector<ivec4> triangles;		// Triangles
	vector<int> halfedges;			// Halfedges
	vector<ivec4> colors;				// Colors per Triangle

	int NP;											// Number of Points
	vector<vec2> points;				// Set of Points
	vector<vec2> originpoints;	// Original Point Position

	ofstream out;								// Output-Stream
	ifstream in;								// Input-Stream

	triangulation(){						// Simple 2-Triangle Constructor

		points.emplace_back(-RATIO,-1);
		points.emplace_back(-RATIO, 1);
		points.emplace_back( RATIO,-1);
		points.emplace_back( RATIO, 1);

		triangles.emplace_back(0, 1, 2, 0);
		triangles.emplace_back(2, 1, 3, 0);

		colors.emplace_back(0,0,0,1);
		colors.emplace_back(0,0,0,1);

		halfedges.push_back(-1);
		halfedges.push_back( 3);
		halfedges.push_back(-1);

		halfedges.push_back( 1);
		halfedges.push_back(-1);
		halfedges.push_back(-1);

		NP = points.size();
		NT = triangles.size();
		colors.resize(MAXT);

		originpoints = points;

	}

	~triangulation(){
		if(out.is_open()) out.close();
		if(in.is_open()) in.close();
	}

	float angle( int ha );											// Compute the Angle Opposite of Halfedge
	float hlength( int ha );										// Length of Edge / Halfedge
	static bool boundary( vec2 p );							// Check whether Point is on domain Boundary
	int boundary( int t );											// Count Number of Points on Boundary for Triangle

	bool eraset( int t, bool );									// Erase Triangle (with Boundary Half-Edge alteration)
	bool erasep( int p );												// Erase Point

	bool prune( int ta );												// Prune Boundary Triangle
	bool flip( int ha, float );									// Flip Triangulation Edge (with Minangle)
	bool collapse( int ha );										// Collapse Triangulation Edge
	bool split( int ta );												// Split Triangle
	bool optimize();														// Optimization Procedure Wrapper

	void warp( vector<vec2>& points );					// Warp Pointvector from originpoints->points using barycentrics
	void reversewarp( vector<vec2>& points );		// Warp Pointvector from points->originpoints using barycentrics

};

size_t triangulation::MAXT = (2 << 18);				// Maximum number of Triangles (for Memory Reservation)

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
												Triangulation Direct Modifiers
================================================================================
*/

bool triangulation::eraset( int t, bool adjusth = true ){

	if(t >= triangles.size())						//Sanity Check
		return false;

	if( adjusth ){

		int h0 = halfedges[3*t+0];				// Exterior Halfedge References
		int h1 = halfedges[3*t+1];
		int h2 = halfedges[3*t+2];

		if(h0 >= 0) halfedges[h0] = -1;		// Remove References
		if(h1 >= 0) halfedges[h1] = -1;
		if(h2 >= 0) halfedges[h2] = -1;

	}

	// Delete the Triangle, DONT Delete Vertices

	triangles.erase( triangles.begin() + t );
	halfedges.erase( halfedges.begin() + 3*t, halfedges.begin() + 3*(t + 1) );
	NT--;

	for(auto& h: halfedges)							// Fix Halfedge Indexing
		if(h >= 3*(t+1)) h -= 3;

	return true;

}

bool triangulation::erasep( int p ){

	if(p >= points.size())
		return false;

	points.erase( points.begin() + p );

	for(auto& t: triangles){
		if( t.x >= p ) t.x--;
		if( t.y >= p ) t.y--;
		if( t.z >= p ) t.z--;
	}

	NP--;

	return true;

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

	// Erase

	return eraset( ta );

}

// Delaunay Flipping

bool triangulation::flip( int ha, float minangle = PI ){

	if(ha < 0) return false;

	int hb = halfedges[ha];		// Opposing Half-Edge
	int ta = ha/3;						// First Triangle
	int tb = hb/3;						// Second Triangle

	if(hb < 0)	return false;	// No Opposing Half-Edge

	// Confirm Convexity: Two-Line Segments Intersect

	const function<bool(vec2, vec2, vec2)> ccw = [](vec2 A, vec2 B, vec2 C){
		return (C.y-A.y) * (B.x-A.x) > (B.y-A.y) * (C.x-A.x);
	};

	vec2 A = points[triangles[ta][(ha+0)%3]];
	vec2 B = points[triangles[tb][(hb+0)%3]];

	vec2 C = points[triangles[ta][(ha+2)%3]];
	vec2 D = points[triangles[tb][(hb+2)%3]];

	if(ccw(A,C,D) == ccw(B,C,D) || ccw(A,B,C) == ccw(A,B,D))
		return false;

	// Continue...

	float aa = angle(ha);
	float ab = angle(hb);

	if(aa + ab < minangle)
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

	return true;

}

// Half-Edge Collapse

bool triangulation::collapse( int ha ){

	if(ha < 0) return false;  // Non-Half-Edge
	int ta = ha/3;						// First Triangle

	int ia = triangles[ta][(ha+0)%3];
	int ib = triangles[ta][(ha+1)%3];

	if(length(points[ia] - points[ib]) > 0.01)
		return false;

	vec2 vn;
	if(boundary(points[ia]) && boundary(points[ib])){
		vn = 0.5f*(points[ia] + points[ib]);
	}
	else if(boundary(points[ia])){
		vn = points[ia];
	}
	else if(boundary(points[ib])){
		vn = points[ib];
	}
	else 	vn = 0.5f*(points[ia] + points[ib]);

	int in = points.size();
	points.push_back(vn);
	NP++;

	// Retrieve Exterior Half-Edge Indices

	int ta1 = halfedges[3*ta+(ha+1)%3];
	int ta2 = halfedges[3*ta+(ha+2)%3];
	if(ta1 >= 0) halfedges[ta1] = ta2;
	if(ta2 >= 0) halfedges[ta2] = ta1;

	// Other Triangle to Delete!

	int hb = halfedges[ha];		// Opposing Half-Edge
	int tb = hb/3;						// Second Triangle
	if(hb >= 0){

		int tb1 = halfedges[3*tb+(hb+1)%3];
		int tb2 = halfedges[3*tb+(hb+2)%3];
		if(tb1 >= 0) halfedges[tb1] = tb2;
		if(tb2 >= 0) halfedges[tb2] = tb1;

		// Erase the Points, Halfedges, Triangles

		eraset( ta, false );
		if(ta < tb) tb--;
		eraset( tb, false );

	}

	else eraset( ta, false );

	// Adcjust all Assignments to the Vertices

	for(auto& t: triangles){

		if(t.x == ia || t.x == ib) t.x = in;
		if(t.y == ia || t.y == ib) t.y = in;
		if(t.z == ia || t.z == ib) t.z = in;

	}

	erasep( ia );
	if(ia < ib) ib--;
	erasep( ib );

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

	return true;

}

/*
================================================================================
														Triangulation Warping
================================================================================
*/

void triangulation::warp( vector<vec2>& npoints){

	if(triangles.empty())	// Can't Warp
		return;

	if(points.empty() || originpoints.empty())
		return;

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

}

void triangulation::reversewarp( vector<vec2>& npoints){

	if(triangles.empty())	// Can't Warp
		return;

	if(points.empty() || originpoints.empty())
		return;

	for(size_t i = 0; i < npoints.size(); i++){		//Iterate over new Points

		if(boundary(npoints[i]))
			continue;

		for(auto& t: triangles){

		//	if(boundary(t) > 0)
		//		continue;

			if(!intriangle(npoints[i], t, points))
				continue;

			npoints[i] = cartesian(barycentric(npoints[i], t, points), t, originpoints);
			break;

		}

	}

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

	pointbuf 		= new Buffer( tpose::triangulation::MAXT, (glm::vec2*)NULL );
	trianglebuf = new Buffer( tpose::triangulation::MAXT, (glm::ivec4*)NULL );
	tcolaccbuf 	= new Buffer( tpose::triangulation::MAXT, (glm::ivec4*)NULL );		// Raw Color
	tcolnumbuf 	= new Buffer( tpose::triangulation::MAXT, (int*)NULL );			// Triangle Size (Pixels)
	tenergybuf 	= new Buffer( tpose::triangulation::MAXT, (int*)NULL );			// Triangle Energy
	penergybuf 	= new Buffer( tpose::triangulation::MAXT, (int*)NULL );			// Vertex Energy
	pgradbuf 		= new Buffer( tpose::triangulation::MAXT, (glm::ivec2*)NULL );
	tnringbuf 	= new Buffer( tpose::triangulation::MAXT, (int*)NULL );

	terr = new int[tpose::triangulation::MAXT];
	perr = new int[tpose::triangulation::MAXT];
	cn 	= new int[tpose::triangulation::MAXT];
	col = new glm::ivec4[tpose::triangulation::MAXT];

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

void upload( tpose::triangulation* tr, bool uploadcolor = true ){

	pointbuf->fill(tr->points);
	trianglebuf->fill(tr->triangles);

	if(uploadcolor){
		int nc = 0;
		for(size_t k = 0; k < tr->NT; k++){
			for(size_t i = 0; i < 13; i++)
				col[i*tr->NT + nc] = tr->colors[k];
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

float geterr( tpose::triangulation* tr ){

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

float gettoterr( tpose::triangulation* tr ){

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

	return abs(toterr);

}

int maxerrid( tpose::triangulation* tr ){

	maxerr = 0;
	int tta = -1;
	for(size_t i = 0; i < tr->NT; i++){
	//	if(cn[13*i] <= 0) continue;
		float err = 0.0f;
		err += abs(terr[i]);
//		err += perr[tr->triangles[i].x];
//		err += perr[tr->triangles[i].y];
//		err += perr[tr->triangles[i].z];
//cout<<"ERR "<<err<<endl;
		if(sqrt(err) > maxerr){
			maxerr = sqrt(err);
			tta = i;
		}
	}

	return tta;

}

} //End of Namespace


#endif
