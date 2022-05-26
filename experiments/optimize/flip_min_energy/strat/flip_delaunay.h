// Delaunay Flip

if( tri::geterr(&tr) < 1E-3 ){

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

  int ha = 3*ta + 0;
  float maxangle = tr.angle( ha );
  if(tr.angle( ha + 1 ) > maxangle)
    maxangle = tr.angle( ++ha );
  if(tr.angle( ha + 1 ) > maxangle)
    maxangle = tr.angle( ++ha );

  tr.flip(ha, tri::PI);

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
