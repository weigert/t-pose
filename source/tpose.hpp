/*
================================================================================
              t-pose: 2D Triangulations for 3D Multiview Geometry
================================================================================
*/

#ifndef TPOSE
#define TPOSE

#ifdef TINYENGINE_UTILITIES

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

#endif
