// Only Single Energy Improving Flips! But also max angle! But no angle restricioon!

/*

  Flip-Set:

  Order by energy, then slowly work off the set removing elements that are not in the set as I go.
  Meanwhile, remember which nodes were flipped so I can flip them back if the energy is unfavorable.

  Do this in its entirety.


  Set of all triangles with energies

  Sort set by energies

  Try flipping all triangles in order, removing triangles that are "not flippable"

  for flipped triangles, store the energies

  check the flipped set to see if its improved or not

  flip' em back

*/

// Sortable Container with Energies and Triangles




// Take the triangles and sort them by their energy??

if( tri::geterr(&tr) < 1E-3 ){

  struct setsort {
    bool operator () (const std::pair<int, float>& lhs, const std::pair<int, float>& rhs) const {
        return lhs.second > rhs.second;
     }
  };

  // Create the sorted set of max-angle half-edges

  std::set<std::pair<int, float>, setsort> hset;
  for(int t = 0; t < tr.triangles.size(); t++){

    if( tr.halfedges[ 3*t + 0 ] >= 0 )
      hset.emplace( 3*t + 0, tri::terr[t] + tri::terr[tr.halfedges[ 3*t + 0 ]/3] );
    if( tr.halfedges[ 3*t + 1 ] >= 0 )
      hset.emplace( 3*t + 1, tri::terr[t] + tri::terr[tr.halfedges[ 3*t + 1 ]/3] );
    if( tr.halfedges[ 3*t + 2 ] >= 0 )
      hset.emplace( 3*t + 2, tri::terr[t] + tri::terr[tr.halfedges[ 3*t + 2 ]/3] );

  }

  std::set<int> nflipset;         // Halfedges NOT to flip!
  std::map<int, float> hflipset;  // Halfedges to Flip

  // Iterate over the sorted triangles

  for(auto& h: hset){

    if(nflipset.contains(h.first))  // Non-Flip Halfedge
      continue;

    if( tr.halfedges[h.first] < 0 ) // No Opposing Halfedge
      continue;

    if(nflipset.contains(tr.halfedges[h.first]))  // Opposing Half-Edge Not Flippable
      continue;

    // Compute Energy for this Half-Edge Pair

    hflipset[ h.first ] = h.second;

    // Add all Half-Edges of Both Triangles to the Non-Flip Set

    int ta = h.first/3;
    int tb = tr.halfedges[ h.first ]/3;

    nflipset.emplace( 3*ta + 0 );
    nflipset.emplace( 3*ta + 1 );
    nflipset.emplace( 3*ta + 2 );
    nflipset.emplace( 3*tb + 0 );
    nflipset.emplace( 3*tb + 1 );
    nflipset.emplace( 3*tb + 2 );

  }

  // Flip the flip set, then check the energy, and flip those that don't work back!

  for(auto& h: hflipset)
    tr.flip( h.first, 0.0f );

  tri::upload(&tr, false);
  computecolors();
  doenergy();

  tri::tenergybuf->retrieve((13*tr.NT), tri::terr);

  for(auto& h: hflipset){

    if( tri::terr[ h.first/3 ] + tri::terr[ (tr.halfedges[ h.first ])/3 ] > h.second )
      tr.flip( h.first, 0.0f ); 	//Flip it Back, Split

  }

  tri::upload(&tr, false);
  computecolors();
  doenergy();
  tri::tenergybuf->retrieve((13*tr.NT), tri::terr);

  int tta = tri::maxerrid(&tr);
  if(tta >= 0 && tr.split(tta))
    updated = true;

}

// Prune Flat Boundary Triangles

for(size_t ta = 0; ta < tr.NT; ta++)
if(tr.boundary(ta) == 3)
  if(tr.prune(ta)) updated = true;

// Attempt a Delaunay Flip on a Triangle's Largest Angle

for(size_t ta = 0; ta < tr.NT; ta++){

  if(tr.angle( 3*ta + 0 ) > 0.8*tri::PI)
    tr.flip( 3*ta + 0, 0.0 );
  if(tr.angle( 3*ta + 1 ) > 0.8*tri::PI)
    tr.flip( 3*ta + 1, 0.0 );
  if(tr.angle( 3*ta + 2 ) > 0.8*tri::PI)
    tr.flip( 3*ta + 2, 0.0 );

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
