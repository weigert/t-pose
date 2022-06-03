
	/*

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

		dist2DA.push_back(length(trWA.originpoints[i] - trA.points[i]));

		vec2 pX = trA.originpoints[i];
		vec2 pY = trA.points[i];

		if(dist2DA.back() > 0.0000002)
			continue;

		if( tri::triangulation::boundary(pX)
		||	tri::triangulation::boundary(pY) )
			continue;

		if(pX.x < -tri::RATIO/2.0) continue;
		if(pX.x > tri::RATIO/2.0) continue;
		if(pX.y < -1.0/2.0) continue;
		if(pX.y > 1.0/2.0) continue;
		if(pY.x < -tri::RATIO/2.0) continue;
		if(pY.x > tri::RATIO/2.0) continue;
		if(pY.y < -1.0/2.0) continue;
		if(pY.y > 1.0/2.0) continue;

		matchX.emplace_back(T*vec3(pX.x, pX.y, 1));
		matchY.emplace_back(T*vec3(pY.x, pY.y, 1));
		weights.emplace_back(1.0f/length(trWA.originpoints[i] - trA.points[i]));

	}

	for(size_t i = 0; i < trB.NP; i++){

		dist2DB.push_back(length(trWB.originpoints[i] - trB.points[i]));

		vec2 pX = trB.points[i];
		vec2 pY = trB.originpoints[i];

		tempX.emplace_back(T*vec3(pX.x, pX.y, 1));
		tempY.emplace_back(T*vec3(pY.x, pY.y, 1));

		if(dist2DB.back() > 0.0000002)
			continue;

		if( tri::triangulation::boundary(pX)
		|| 	tri::triangulation::boundary(pY) )
			continue;

		if(pX.x < -tri::RATIO/2.0) continue;
		if(pX.x > tri::RATIO/2.0) continue;
		if(pX.y < -1.0/2.0) continue;
		if(pX.y > 1.0/2.0) continue;
		if(pY.x < -tri::RATIO/2.0) continue;
		if(pY.x > tri::RATIO/2.0) continue;
		if(pY.y < -1.0/2.0) continue;
		if(pY.y > 1.0/2.0) continue;

		matchX.emplace_back(T*vec3(pX.x, pX.y, 1));
		matchY.emplace_back(T*vec3(pY.x, pY.y, 1));
		weights.emplace_back(1.0f/length(trWB.originpoints[i] - trB.points[i]));

	//	cout<<"W"<<weights.back()<<endl;

	}

	*/


  	Matrix3f K = unp::Camera();
  //	Matrix3f F = unp::F_Sampson(matchX, matchY, weights);
  //	Matrix3f F = unp::F_LMEDS(matchX, matchY);
  //	Matrix3f F = unp::F_RANSAC(matchX, matchY);

  	Matrix3f F;
  	F << -0.936791,      2.24,  -26.1548,
    1.03284,   1.77101,  -13.5179,
    26.3407,   10.5485,         1;


  	cout<<"Fundamental Matrix: "<<F<<endl;

  	point3D = unp::triangulate(F, K, tempX, tempY);

  	point3Dbuf.fill(point3D);
