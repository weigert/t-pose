// triangulate.h

using namespace glm;
using namespace std;

struct Triangle : Model {
	Buffer vert;
	Triangle():Model({"vert"}),
	vert({1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f}){
		bind<vec3>("vert", &vert);
		SIZE = 3;
	}
};

struct TLineStrip : Model {
	Buffer vert;
	TLineStrip():Model({"vert"}),
	vert({1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f}){
		bind<vec3>("vert", &vert);
		SIZE = 4;
	}
};


Buffer* trianglebuf;
Buffer* pointbuf;

vector<vec2> points;			//Coordinates for Delaunation
vector<ivec4> triangles;	//Triangle Point Indexing
vector<int> halfedges;	//Triangle Halfedge Indexing

int NPoints;
int KTriangles;
int NTriangles;

void initialize( const int K = 1024 ){

  // Create a Delaunator!

	points.emplace_back(-1.2/0.8f,-1);
	points.emplace_back(-1.2/0.8f, 1);
	points.emplace_back( 1.2/0.8f,-1);
	points.emplace_back( 1.2/0.8f, 1);

	while(points.size() < K/2)
		sample::disc(points, K, vec2(-12.0f/8.0f, -1.0f), vec2(12.0f/8.0f, 1.0f));

	vector<double> coords;					//Coordinates for Delaunation
  for(size_t i = 0; i < points.size(); i++){
    coords.push_back(points[i].x);
    coords.push_back(points[i].y);
  }

	delaunator::Delaunator d(coords);			//Compute Delaunay Triangulation

	pointbuf = new Buffer(points);

  for(size_t i = 0; i < d.triangles.size()/3; i++){
		triangles.emplace_back(d.triangles[3*i+0], d.triangles[3*i+1], d.triangles[3*i+2], 0);
		halfedges.push_back(d.halfedges[3*i+0]);
		halfedges.push_back(d.halfedges[3*i+1]);
		halfedges.push_back(d.halfedges[3*i+2]);
	}

	trianglebuf = new Buffer(triangles);

}

// Triangle Helper Functions

// Angle Opposite to Half-Edge

float angle( int ha ){

	int ta = ha/3;	// Triangle Index

	vec2 paa = points[triangles[ta][(ha+0)%3]];
	vec2 pab = points[triangles[ta][(ha+1)%3]];
	vec2 pac = points[triangles[ta][(ha+2)%3]];
	if(length(paa - pac) == 0) return 0;
	if(length(pab - pac) == 0) return 0;
	return acos(dot(paa - pac, pab - pac) / length(paa - pac) / length(pab - pac));

}

// Length of Half-Edge

// Angle Opposite to Half-Edge

float hlength( int ha ){

	int ta = ha/3;	// Triangle Index

	vec2 paa = points[triangles[ta][(ha+0)%3]];
	vec2 pab = points[triangles[ta][(ha+1)%3]];
	return length(pab - paa);

}

int boundary( int t ){

	int nboundary = 0;

	if(points[triangles[t].x].x <= -1.2/0.8f
	|| points[triangles[t].x].y <= -1
	|| points[triangles[t].x].x >=  1.2/0.8f
	|| points[triangles[t].x].y >=  1) nboundary++;

	if(points[triangles[t].y].x <= -1.2/0.8f
	|| points[triangles[t].y].y <= -1
	|| points[triangles[t].y].x >=  1.2/0.8f
	|| points[triangles[t].y].y >=  1) nboundary++;

	if(points[triangles[t].z].x <= -1.2/0.8f
	|| points[triangles[t].z].y <= -1
	|| points[triangles[t].z].x >=  1.2/0.8f
	|| points[triangles[t].z].y >=  1) nboundary++;

	return nboundary;

}

// Topological Alterations

/*

		Topological Optimization:
			- Collapse Vertices (Thin Triangles)
			- Delaunay Flip (Flat Triangles)
			- Centroid Split (Bad Triangles)

*/

// Remove Flat Boundary Triangle

bool prune( int ta ){

	// Get Exterior Half-Edges

	int ta0 = halfedges[3*ta+0];
	int ta1 = halfedges[3*ta+1];
	int ta2 = halfedges[3*ta+2];

	// Check Non-Prunable

	if(ta0 >= 0 && ta1 >= 0 && ta2 >= 0)
		return false;

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

	KTriangles--;
	NTriangles = (1+12)*KTriangles;

	cout<<"PRUNED"<<endl;

	return true;

}

// Delaunay Flipping

void flip( int ha ){

	int hb = halfedges[ha];		// Opposing Half-Edge
	int ta = ha/3;						// First Triangle
	int tb = hb/3;						// Second Triangle

	if(hb < 0)	return;				// No Opposing Half-Edge

	float aa = angle(ha);
	float ab = angle(hb);

	if(aa + ab < 3.14159265f + 1E-8)
		return;

	if(aa < 1E-8 || ab <= 1E-8)
		return;

	// Retrieve Half-Edge Indices

	int ta0 = halfedges[3*ta+(ha+0)%3];	//h3
	int ta1 = halfedges[3*ta+(ha+1)%3];	//h6
	int ta2 = halfedges[3*ta+(ha+2)%3];	//h7

	int tb0 = halfedges[3*tb+(hb+0)%3];	//h0
	int tb1 = halfedges[3*tb+(hb+1)%3];	//h8
	int tb2 = halfedges[3*tb+(hb+2)%3];	//h9

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

}

// Half-Edge Collapse

void collapse( int ha ){

	int hb = halfedges[ha];		// Opposing Half-Edge
	int ta = ha/3;						// First Triangle
	int tb = hb/3;						// Second Triangle

	if(hb < 0)	return;				// No Opposing Half-Edge

	// Retrieve Exterior Half-Edge Indices

	int ta1 = halfedges[3*ta+(ha+1)%3];	//a1
	int ta2 = halfedges[3*ta+(ha+2)%3];	//a2

	int tb1 = halfedges[3*tb+(hb+1)%3];	//b1
	int tb2 = halfedges[3*tb+(hb+2)%3];	//b2

	// Get the Two Vertices, Merge (Add new Point)

	int ia = triangles[ta][(ha+0)%3];
	int ib = triangles[tb][(hb+0)%3];

	if(ia < 4 || ib < 4)	//we don't collapse edges, my friend
		return;

	if(length(points[ia] - points[ib]) > 0.1)
		return;

	cout<<"COLLAPSE 1"<<endl;

	vec2 vn = 0.5f*(points[ia] + points[ib]);
	int in = points.size();
	points.push_back(vn);

	// Adjust all Assignments to the Vertices

	for(auto& t: triangles){

		if(t.x == ia || t.x == ib) t.x = in;
		if(t.y == ia || t.y == ib) t.y = in;
		if(t.z == ia || t.z == ib) t.z = in;

	}

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
		if(tt.x > ia) t.x--;
		if(tt.y > ia) t.y--;
		if(tt.z > ia) t.z--;
		if(tt.x > ib) t.x--;
		if(tt.y > ib) t.y--;
		if(tt.z > ib) t.z--;

	}

	for(auto& h: halfedges){

		int th = h;
		if(th >= 3*(ta+1)) h -= 3;
		if(th >= 3*(tb+1)) h -= 3;

	}

	KTriangles -= 2;
	NTriangles = (1+12)*KTriangles;
	NPoints--;

}

// Triangle Splitting

void split( int ta ){

	ivec4 tca = triangles[ta];	//Triangle Verticees

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

	halfedges.resize(triangles.size()*3);

	halfedges[3*ta + 0] = tax;
	halfedges[3*ta + 1] = 3*tb + 2;
	halfedges[3*ta + 2] = 3*tc + 1;

	halfedges[3*tb + 0] = tay;
	halfedges[3*tb + 1] = 3*tc + 2;
	halfedges[3*tb + 2] = 3*ta + 1;

	halfedges[3*tc + 0] = taz;
	halfedges[3*tc + 1] = 3*ta + 2;
	halfedges[3*tc + 2] = 3*tb + 1;

	// Exterior Half-Edge Refrence Shift

	if(tax >= 0)	halfedges[tax] = 3*ta + 0;
	if(tay >= 0)	halfedges[tay] = 3*tb + 0;
	if(taz >= 0)	halfedges[taz] = 3*tc + 0;

	// Update Numbers

	KTriangles += 2;
	NTriangles = (1+12)*KTriangles;
	NPoints++;

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

void optimize(){

	// Prune Flat Boundary Triangles

	for(size_t ta = 0; ta < triangles.size(); ta++)
	if(boundary(ta) == 3)
		prune(ta);

	// Attempt a Delaunay Flip on a triangle's Largest Angle

	for(size_t ta = 0; ta < triangles.size(); ta++){

		int ha = 3*ta + 0;
		float maxangle = angle( ha );
		if(angle( ha + 1 ) > maxangle)
			maxangle = angle( ++ha );
		if(angle( ha + 1 ) > maxangle)
			maxangle = angle( ++ha );
		flip(ha);

	}

	/*

	// Collapse Small Edges

	for(size_t ta = 0; ta < triangles.size(); ta++){

		int ha = 3*ta + 0;
		float minlength = hlength( ha );
		if(hlength( ha + 1 ) <= minlength){
			ha++; minlength = hlength( ha + 1 );
		}
		if(hlength( ha + 1 ) <= minlength){
			ha++; minlength = hlength( ha + 1 );
		}
		collapse(ha);

	}

	*/

	/*



	*/



		/*
		int ta = rand()%KTriangles;					// Pick a Random Triangle
		int ha = 3*ta + 0;
		float maxangle = angle( ha );
		if(angle( ha + 1 ) >= maxangle){
			ha++; maxangle = angle( ha + 1 );
		}
		if(angle( ha + 1 ) >= maxangle){
			ha++; maxangle = angle( ha + 1 );
		}
		flip(ha);
		*/



}



	/*
	int maxerr = 0;
	for(size_t i = 0; i < KTriangles; i++){
		if(cn[i] == 0) continue;
		if(sqrt(err[i]) >= maxerr)
			maxerr = sqrt(err[i]);
	}


	cout<<"MAXERR "<<maxerr<<endl;
	if(maxerr <= 10)
		return;



	for(size_t t = 0; t < KTriangles; t++){

		if(cn[t] == 0) continue;
		if(sqrt(err[t]) < 0.9*maxerr)
			continue;

		if(cn[t] < 15)
			continue;

		cout<<"ERR "<<t<<" "<<sqrt(err[t])<<" "<<cn[t]<<endl;

		*/
