
When warping it probably makes sense to do so hierarchically.

This means take a coarse triangulation, warp the triangulation, take any points from the next trianguation
INSIDE every triangle and warp them accordingly, then fit
do so recursively until complete
