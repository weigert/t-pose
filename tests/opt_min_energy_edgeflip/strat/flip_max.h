// Only Single Energy Improving Flips! But also max angle! But no angle restricioon!

/*

  Flip-Set:

  Order by energy, then slowly work off the set removing elements that are not in the set as I go.
  Meanwhile, remember which nodes were flipped so I can flip them back if the energy is unfavorable.

  Do this in its entirety.

*/

// I have the energy:

//std::list<int> triangles;
//for(size_t i = 0; i < tr.triangles.size(); i++){
//  triangles.push_front(i);
//}

// Take the triangles and sort them by their energy??

if( tri::geterr(&tr) < 1E-3 ){

  int tta = tri::maxerrid(&tr);
  if(tta >= 0){

    int ha = 3*tta + 0;

    float maxangle = tr.angle( ha );
    if(tr.angle( ha + 1 ) > maxangle)
      maxangle = tr.angle( ++ha );
    if(tr.angle( ha + 1 ) > maxangle)
      maxangle = tr.angle( ++ha );

    if(tr.halfedges[ ha ] >= 0){

      float energy = tri::terr[ tta ] + tri::terr[ (tr.halfedges[ ha ])/3 ];

      // Try Flip Regardless of Angle

      if( tr.flip( ha, 0.0f ) ){

      	tri::upload(&tr, false);
        computecolors();
        doenergy();

        tri::tenergybuf->retrieve((13*tr.NT), tri::terr);

        // if Energy Fails, Flip it Back, Split it

        if( tri::terr[ tta ] + tri::terr[ (tr.halfedges[ ha ])/3 ] > energy ){
          tr.flip( ha, 0.0f ); 	//Flip it Back
          if(tr.split( tta ))
            updated = true;
        }

      }

      else {
        if(tr.split( tta ))
          updated = true;
      }

    }

    else {
      if(tr.split( tta ))
        updated = true;
    }

  }

}

// Prune Flat Boundary Triangles

for(size_t ta = 0; ta < tr.NT; ta++)
if(tr.boundary(ta) == 3)
  if(tr.prune(ta)) updated = true;

// Attempt a Delaunay Flip on a Triangle's Largest Angle

for(size_t ta = 0; ta < tr.NT; ta++){

  int ha = 3*ta + 0;
  float maxangle = tr.angle( ha );
  if(tr.angle( ha + 1 ) > maxangle)
    maxangle = tr.angle( ++ha );
  if(tr.angle( ha + 1 ) > maxangle)
    maxangle = tr.angle( ++ha );

  if(maxangle >= 0.9*tri::PI)
    tr.flip(ha, 0.5*tri::PI);
  else tr.flip(ha, 1.5f*tri::PI);

}

// Collapse Small Edges

for(size_t ta = 0; ta < tr.triangles.size(); ta++){

  int ha = 3*ta + 0;
  float minlength = tr.hlength( ha );
  if(tr.hlength( ha + 1 ) < minlength)
    minlength = tr.hlength( ++ha );
  if(tr.hlength( ha + 1 ) < minlength)
    minlength = tr.hlength( ++ha );
  if(tr.collapse(ha))
    updated = true;

}
